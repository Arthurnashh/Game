// Poseidon - shape management
// (C) 1998, SUMA

#include "../wpch.hpp"
#include "shape.hpp"

#include "../serializeBinExt.hpp"
#include "../paramFileExt.hpp"
#include <El/QStream/qbStream.hpp>
#include <El/Common/perfProf.hpp>
#include <Es/ReportStack/reportStack.hpp>
#include "../engine.hpp"
#include "specLods.hpp"
#include "../txtPreload.hpp"
#include "material.hpp"
#include "mapTypes.hpp"
#include "../rtAnimation.hpp"
#include "../animation.hpp"
#include "../progress.hpp"

#include <Es/Memory/normalNew.hpp>

#if _ENABLE_PHYSICS
#include "Physics\enf_worldphysics.h"
#endif


ShapeRefManaged::ShapeRefManaged(Shape *shape)
:base(shape)
{
  _locked = 0;
  #if _ENABLE_REPORT
    _lockedCB = AtomicInt(0);
  #endif
}

void ShapeRefManaged::Lock()
{
  if (GEngine->IsInCBScope())
  {
    // when locking inside of the CB rendering, do not actually do anything
    #if _ENABLE_REPORT
      _lockedCB++;
    #endif
    // verify the data are really there
    Assert(IsLoaded());
    return;
  }
  #if _ENABLE_REPORT
    // when we are manipulating real count, we should be outside of CB, and CB should have clean up
    DoAssert(_lockedCB==0);
  #endif
  if (_locked++==0)
  {
    Load();
  }
}
void ShapeRefManaged::Unlock()
{
  if (GEngine->IsInCBScope())
  {
    // when locking inside of the CB rendering, do not actually do anything
    #if _ENABLE_REPORT
      _lockedCB--;
      DoAssert(_lockedCB>=0);
    #endif
    return;
  }
  #if _ENABLE_REPORT
    // when we are manipulating real count, we should be outside of CB, and CB should have clean up
    DoAssert(_lockedCB==0);
  #endif
  DoAssert(_locked>0);
  if (--_locked==0)
  {
    Unload();
  }
}

bool ShapeRefManaged::IsLocked() const
{
  if (_locked>0) return true;
  if (GEngine->IsInCBScope())
  {
    // when locks are not done, we cannot test them
    // note: this call is used mostly for assertions
    #if _ENABLE_REPORT
    return _lockedCB>0;
    #else
    return true;
    #endif
  }
  return false;
}


void ShapeRefManaged::Load()
{
  DoAssert(!GEngine->IsInCBScope());
  Shapes.ShapeLoaded(this);
}
void ShapeRefManaged::Unload()
{
  Shapes.ShapeUnloaded(this);
}

ShapeRefManaged::~ShapeRefManaged()
{
  AssertMainThread();
  #if _ENABLE_REPORT
    // make sure there is no trailing Lock done during CB recording
    Assert(_lockedCB==0);
  #endif
}



//! loadable shape - can be unloaded when not used
class ShapeRefLoadable: public ShapeRefManaged
{
  typedef ShapeRefManaged base;

  //! some properties need to be stored permanently for quick access
  int _nFaces;
  mutable int _nVertices; 
  mutable float _faceArea;

  PackedColor _color;
  int _special;
  ClipFlags _orHints;

  //! source location stored
  QFBankHandle _handle;
  int _version;
  #ifdef _M_PPC
  SerializeBinStream::Endian _endianState;
  #endif
  /// when true, LZO compression is used in LoadCompressed, SaveCompressed etc., otherwise LZN (SSCompress) is used
  bool _lzoCompression;
  /** only new models contain _nVertices and _area, for older ones we need to use heuristics until loaded */
  mutable bool _nVerticesAndAreaComputed;

  /// which shape this belongs to
  LODShape *_lodShape;
  int _level;
  
  /// is there any skeleton present in the model?
  bool _hasSkeleton;

public:

  /// create a loadable ref from the level itself
  ShapeRefLoadable(
    const QFBankHandle &handle, LODShape *lShape, int level, Shape *shape,
    int version
#ifdef _M_PPC
    , SerializeBinStream::Endian endianState
#endif
    , bool lzoCompression
  );
  /// create a loadable ref and serialized it from a stream
  ShapeRefLoadable(
    const QFBankHandle &handle, LODShape *lShape, int level, SerializeBinStream &f,
    int version
#ifdef _M_PPC
    , SerializeBinStream::Endian endianState
#endif
    , bool lzoCompression
  );

  virtual bool IsLoaded() const;
  virtual bool RequestLoading(bool load) const;

  virtual void DoLoad();
  virtual void DoUnload();
  virtual void OnGeometryLoaded(Shape *shape) const;
  
  void EstimateVerticesAndArea(int nFaces, const LODShape *lShape);
  size_t GetMemoryControlled() const ;

  int NFaces() const {return _nFaces;}
  int NVertices() const {return _nVertices;}
  float FaceArea() const {return _faceArea;}
  bool NVerticesAndAreaKnown() const {return _nVerticesAndAreaComputed;}

  bool HasSkeleton() const {return _hasSkeleton;}
  PackedColor GetColor() const {return _color;}
  ClipFlags GetOrHints() const
  {
    Assert(!_ref || (_ref->GetOrHints()&~ClipUserMask)==(_orHints&~ClipUserMask));
    return _orHints;
  }
  int Special() const
  {
    Assert(!_ref || _ref->Special()==_special);
    return _special;
  }

  USE_FAST_ALLOCATOR
};


#include <Es/Memory/debugNew.hpp>


DEFINE_FAST_ALLOCATOR(ShapeRefLoadable)

void ShapeRefLoadable::EstimateVerticesAndArea(int nFaces, const LODShape *lShape)
{
  Assert(!_nVerticesAndAreaComputed);
  _nVerticesAndAreaComputed = false;
  _nVertices = nFaces*2;
  // overestimation could prevent the LOD from ever loading, we rather underestimate
  _faceArea = lShape->EstimateArea(1.0f); //*8.0f; // 4x is sphere against the projected circle, we can assume some overdraw as well
}

ShapeRefLoadable::ShapeRefLoadable(
  const QFBankHandle &handle, LODShape *lShape, int level, Shape *shape, int version
#ifdef _M_PPC
  , SerializeBinStream::Endian endianState
#endif
  , bool lzoCompression
)
:base(shape),_handle(handle)
{
  Assert(shape);
  _nVerticesAndAreaComputed = shape->IsGeometryLoaded() && shape->IsLoadLocked();
  _nFaces = shape->NFaces();
  if (_nVerticesAndAreaComputed)
  {
    _nVertices = shape->NVertex();
    _faceArea = shape->TotalFaceArea();
  }
  else
  {
    EstimateVerticesAndArea(_nFaces,lShape);
  }
  _color = shape->GetColor();
  _special = shape->Special();
  _orHints = shape->GetOrHints();
  _version = version;
#ifdef _M_PPC
  _endianState = endianState;
#endif
  _lzoCompression = lzoCompression;
  _lodShape = lShape;
  _level = level;
  _hasSkeleton = shape->GetSubSkeletonSize()>0;
}

#if NAMED_SEL_STATS
RString NamedSelCreateContext;
#endif


ShapeRefLoadable::ShapeRefLoadable(
  const QFBankHandle &handle, LODShape *lShape, int level, SerializeBinStream &f,
  int version
#ifdef _M_PPC
  , SerializeBinStream::Endian endianState
#endif
  , bool lzoCompression
)
:base(NULL),_handle(handle)
{
  _version = version;
#ifdef _M_PPC
  _endianState = endianState;
#endif
  _lzoCompression = lzoCompression;
  _lodShape = lShape;
  _level = level;

  /// corresponds to LODShape::SerializeBin - seach for LOAD_REF
  _nFaces = f.LoadInt();
  _color = PackedColor(f.LoadInt());
  _special = f.LoadInt();
  _orHints = f.LoadInt();
  if (_version>=39)
  {
    _hasSkeleton = f.LoadChar()!=0;
  }
  else
  {
    // unless proven otherwise, assume skeleton exists as long as LODShape has one
    _hasSkeleton = lShape->GetSkeleton()!=NULL;
  }
  if (_version>=51)
  {
    _nVertices = f.LoadInt();
    f.LoadLittleEndian(&_faceArea,1,sizeof(_faceArea));
    _nVerticesAndAreaComputed = true;
}
  else
  {
    _nVerticesAndAreaComputed = false;
    EstimateVerticesAndArea(_nFaces,lShape);
  }
}

class PLPushBuffer;
void ShapeRefLoadable::DoLoad()
{
  PROFILE_SCOPE(slDoL);
  // we need to load a new shape - this means creating it and loading it
  if (IsLoaded())
  {
    Log("Shape already loaded");
    return;
  }

  REPORTSTACKITEM(_lodShape->GetName());
  //_ref.Free();
  Ref<Shape> shape = new Shape;

  QIFStreamB fs;
  fs.open(_handle);
  SerializeBinStream f(&fs);
  f.SetVersion(_version);
  #ifdef _M_PPC
  f.SetEndianState(_endianState);
  #endif
  if (_lzoCompression) f.UseLZOCompression();

  int begGeom = 0, endGeom = 0;
  int begFaces = 0, endFaces = 0;
  {
    PROFILE_SCOPE(slSeB);
    // this should be all we need for loading

    #if NAMED_SEL_STATS
      NamedSelCreateContext = _lodShape->GetName()+"-"+LODShape::LevelName(_lodShape->_resolutions[_level]);
    #endif

    shape->SerializeBin(f,false,begGeom,endGeom,begFaces,endFaces,_lodShape);
    #if NAMED_SEL_STATS
      NamedSelCreateContext = RString();
    #endif

  }
  // adapt handle

  shape->SetLoadHandle(
    QFBankHandle(_handle,begGeom,endGeom),
    QFBankHandle(_handle,begFaces,endFaces),
    _lodShape,_version
    #ifdef _M_PPC
    ,_endianState
    #endif
    ,_lzoCompression
  );

  _ref = shape;
  shape->_trackRef = this;

  // if the shape is not referenced, it is strange - how could it be used?
  Assert(_lodShape->RefCounter()>0);
  // pretend the shape is locked
  // we know it is already in the memory
  _lodShape->TempAddRef();
  TempLock();

  // geometry was not loaded - no need to reverse it
  // we need to reverse what needs to be reversed in the shape
  if (_lodShape->Remarks()&ShapeReversed)
  {
    shape->Reverse();
  }
  if (_version<52)
  {
    _lodShape->CalculateMinMaxVisual();
  }
  // apply autocenter to all positions
  Vector3Val bcenterChange = _lodShape->BoundingCenter()-_lodShape->BoundingCenterOnLoad();
  _lodShape->ApplyBoundingCenter(_level,bcenterChange);
 

  // call loaded handlers
  _lodShape->ShapeLoaded(_level);
  TempUnlock();
  _lodShape->TempRelease();
  //LogF("RefShape %s:%d loaded",cc_cast(_lodShape->GetName()),_level);

  //Log("DoLoad %s:%s",cc_cast(_lodShape->GetName()),cc_cast(_lodShape->LevelName(_level)));
  if (_version<39)
  {
    // hasSkeleton was not serialized - we want it to set now
    _hasSkeleton = shape->GetSubSkeletonSize()>0;
  }
}
void ShapeRefLoadable::DoUnload()
{
  //Log("DoUnload %s:%s",cc_cast(_lodShape->GetName()),cc_cast(_lodShape->LevelName(_level)));
  // pretend the shape is locked
  // we know it is still in the memory, it will be released once we finish
  TempLock();
  _lodShape->TempAddRef();
  _lodShape->ShapeUnloaded(_level);
  _lodShape->TempRelease();
  TempUnlock();
  _ref.Free();
}

void ShapeRefLoadable::OnGeometryLoaded(Shape *shape) const
{
  if(_version<51)
  {
    if(!_nVerticesAndAreaComputed)
    {
      shape->RecalculateFaceArea();
      _nVertices = shape->NVertex();
      _faceArea = shape->TotalFaceArea();
      _nVerticesAndAreaComputed = true;
      _lodShape->RescanLodsArea();
    }
    else
    {
      // data not containing correct face area, we want to supply it
      shape->_faceArea = _faceArea;
    }
  }
}

bool ShapeRefLoadable::IsLoaded() const
{
  return GetRef()!=NULL;
}

bool ShapeRefLoadable::RequestLoading(bool load) const
{
  // note: nobody can have it locked - if it has, we should not be called
  Assert(GetLockCount()==0);
  PROFILE_SCOPE(slReq);
  QIFStreamB fs;
  fs.open(_handle);
  if (fs.PreRead(FileRequestPriority(10)))
  {
    bool ret = true;
    if (_version>=17)
    {
      PROFILE_SCOPE(slPre);
      if (PROFILE_SCOPE_NAME(slPre).IsActive() && _lodShape)
      {
        PROFILE_SCOPE_NAME(slPre).AddMoreInfo(_lodShape->GetName());
      }
      // check if proxies are ready
      SerializeBinStream f(&fs);
      f.SetVersion(_version);
      #ifdef _M_PPC
      f.SetEndianState(_endianState);
      #endif
      if (_lzoCompression) f.UseLZOCompression();
      RefArray<ProxyObject> proxies;
      f.TransferRefArray(proxies);
      for (int i=0; i<_lodShape->_loadHandler.Size(); i++)
      {
        PROFILE_SCOPE(slPrx);
        LODShapeLoadHandler *handler = _lodShape->_loadHandler[i];
        if (!handler->IsShapeReady(_lodShape,_level,proxies.Data(),proxies.Size(),true))
        {
          ret = false;
        }
      }
    }
    if (ret)
    {
      // _locked==0 should not be possible (see Assert above), but to be safer we handle it here anyway
      if (load && GetLockCount()==0)
      {
        // load, but do not increase lock count
        unconst_cast(this)->Load();
        // unload immediately, to place data in cache (nobody has them locked now)
        unconst_cast(this)->Unload();
      }
    }
    return ret;
  }
  return false;
}


size_t ShapeRefLoadable::GetMemoryControlled() const
{
  Shape *shape = GetRef();
  return shape->GetMemoryControlledByShape(false);
}


void NamedProperty::SerializeBin(SerializeBinStream &f)
{
  f.Transfer(_name);
  f.Transfer(_value);
}

ProxyObject::ProxyObject()
{
  boneIndex = SkeletonIndexNone;
  section = -1;
}


void ProxyObject::SerializeBin(SerializeBinStream &f)
{
  f.Transfer(name);
  f.Transfer(trans);
  if (f.IsLoading())
  {
    invTransform=trans.InverseScaled();
  }
  // color/state identical to parent object
  f.Transfer(id);
  f.Transfer(selection); // source selection index
  f.Transfer(boneIndex);
  if (f.GetVersion()>=40)
  {
    f.Transfer(section);
  }
  else if (f.IsLoading())
  {
    section = -1;
  }
}

void AnimationPhase::SerializeBin(SerializeBinStream &f)
{
  f.Transfer(_time);
  f.TransferBasicArray(_points);  
}

const int OldPolyOverhead = sizeof(int)+sizeof(void *);

//! convert old offset to a new one (not including PolyProperties)
class OffsetToIndex: public FindArray<Offset>
{
  public:
  Offset Convert(Offset old) const;
};

Offset OffsetToIndex::Convert(Offset old) const
{
  int index = Find(old);
  if (index<0)
  {
    Fail("Offset not found");
    return Offset(0);
  }
  // new index does not include PolyProperties part
  OffsetAdvance(old,-index*OldPolyOverhead);
  return old;
}

static inline void ReverseVectorCompressed(Vector3Compressed &vc)
{
  Vector3 v = vc.Get();
  v[0] = -v[0],v[2] = -v[2];
  vc.Set(v);
}

void ReverseMinMax( Vector3 *v )
{
  Vector3 min = v[0];
  Vector3 max = v[1];
  // swapped min becomes max
  v[1][0] = -min[0];
  v[1][2] = -min[2];
  v[0][0] = -max[0];
  v[0][2] = -max[2];
}


void Shape::Reverse()
{
  // reverse all positions and normals
  // note: bounding center reversion is non-trivial
  // when using LODShape::Load, all data is reversed before substracting bc
  // reversing can be imagined as applying transformation T
  // any model coordinate position should be
  // T*X-bc
  // actualy it is 
  // 

  int n = NVertex();
  if (IsGeometryLoaded() && n>0)
  {
    for (int i=0; i<n; i++)
    {
      ReverseVector(_data->_pos[i]);
    }
    _data->_norm.ForEach(ReverseVectorCompressed);
    if (IsSTPresent())
    {
      for (int i=0; i<n; i++)
      {
        ReverseVectorCompressed(_data->_st[i].s);
        ReverseVectorCompressed(_data->_st[i].t);
      }
    }
  }

  ReverseMinMax(_minMax);
  ReverseVector(_bCenter);

  // reverse all proxy object matrices
  for (int i=0; i<_proxy.Size(); i++)
  {
    ProxyObject *pobj = _proxy[i];
    //Object *obj = pobj->obj;
    // reverse object transform matrix
    // reverse inverse matrix
    {
      Matrix4 &trans = pobj->trans;
      
      trans.SetPosition(ReverseV(trans.Position()));
      trans.SetDirection(ReverseV(trans.Direction()));
      trans.SetDirectionUp(ReverseV(trans.DirectionUp()));
      trans.SetDirectionAside(ReverseV(trans.DirectionAside()));
      // TODO: FrameBase?
    }

    {
      // reverse inverse matrix
      Matrix4 &trans = pobj->invTransform;

      trans.SetPosition(ReverseV(trans.Position()));
      trans.SetDirection(ReverseV(trans.Direction()));
      trans.SetDirectionUp(ReverseV(trans.DirectionUp()));
      trans.SetDirectionAside(ReverseV(trans.DirectionAside()));
    }
  }

  // reverse all animation data
  for (int i=0; i<_phase.Size(); i++)
  {
    AnimationPhase &phase = _phase[i];
    for (int p=0; p<phase.Size(); p++)
    {
      ReverseVector(phase[i]);
    }
  }
  // if there are any planes, we need to reverse them as well
  if (_plane.Size()>0)
  {
    for (int i=0; i<_plane.Size(); i++)
    {
      Plane &plane = _plane[i];
      // reversing a plane means reversing its normal
      // D (distance from origin) stays the same
      Vector3 n = plane.Normal();
      ReverseVector(n);
      plane.SetNormal(n,plane.D());
    }
  }
}

#include <Es/Memory/normalNew.hpp>

/// load shape from QFStreamB
class ShapeLoadSourceQFStreamB: public IShapeLoadSource
{
  /// handle (file and location) for loading geometry
  QFBankHandle _handle;
  /// handle (file and location) for loading face array
  QFBankHandle _handleFace;
  /** Link cannot be used here, because links are destructed once the shape is cached */
  LODShape *_lShape;
  int _version;
  #ifdef _M_PPC
  SerializeBinStream::Endian _endianState;
  #endif
  /// when true, LZO compression is used in LoadCompressed, SaveCompressed etc., otherwise LZN (SSCompress) is used
  bool _lzoCompression;
  
  public:
  ShapeLoadSourceQFStreamB(
    const QFBankHandle &handle, const QFBankHandle &handleFaces, LODShape *lShape, int version
#ifdef _M_PPC
    , SerializeBinStream::Endian endianState
#endif
    , bool lzoCompression
  )
  :_handle(handle),_handleFace(handleFaces),_lShape(lShape),_version(version)
  #ifdef _M_PPC
  ,_endianState(endianState)
  #endif
  ,_lzoCompression(lzoCompression)
  {
  }
  virtual void Load(Shape *shape);
  virtual bool Request(Shape *shape);
  virtual RString GetDebugName() const;
  virtual Skeleton *GetSkeleton() const;
  
  USE_FAST_ALLOCATOR
};
#include <Es/Memory/debugNew.hpp>

DEFINE_FAST_ALLOCATOR(ShapeLoadSourceQFStreamB)

void ShapeLoadSourceQFStreamB::Load(Shape *shape)
{
  REPORTSTACKITEM(_lShape->GetName());
  #if 0 //_ENABLE_REPORT
    LogF("Loading geometry from %s",cc_cast(_handle.GetDebugName()));
    if (strstr(_handle.GetDebugName(),"mc vojakw2"))
    {
      __asm nop;
    }
  #endif
#ifdef _M_PPC
    shape->LoadVertices(_handle,_lShape, _version, _endianState, _lzoCompression);
    shape->LoadFaces(_handleFace, _lShape, _version, _endianState, _lzoCompression);
#else
    shape->LoadVertices(_handle,_lShape, _version, _lzoCompression);
    shape->LoadFaces(_handleFace, _lShape, _version, _lzoCompression);
#endif
}

bool ShapeLoadSourceQFStreamB::Request(Shape *shape)
{
  #ifdef _M_PPC
  return (
    shape->RequestVertices(_handle,_lShape, _version, _endianState, _lzoCompression) &&
    shape->RequestFaces(_handleFace,_lShape, _version, _endianState, _lzoCompression)
  );
  #else
  return (
    shape->RequestVertices(_handle,_lShape, _version, _lzoCompression) &&
    shape->RequestFaces(_handleFace,_lShape, _version, _lzoCompression)
    );
  #endif
}

Skeleton *ShapeLoadSourceQFStreamB::GetSkeleton() const
{
  return _lShape->GetSkeleton();
}

//////////////////////////////////////////////////////////////////////////

void Shape::DoFaceLoad(SerializeBinStream &f, LODShape *lodShape, int version)
{
  int count = f.LoadInt();
  int size = f.LoadInt();
  // load data to temporary buffer
  // during face loading create offset conversion table
  _face._data = new FaceData;
  if(version==7)
  {
    OffsetToIndex *oconv = new OffsetToIndex;
    oconv->Realloc(count+1);
    _face.ReserveRaw(size);
    for (int i=0; i<count; i++)
    {
      Poly poly;
      int spec = f.LoadInt();
      int ti = f.LoadShort();
      (void)spec;(void)ti;
      // reset IsAnimated / IsAlpha flags based on texture props
      poly.SetN(f.LoadChar());
      for (int i=0; i<poly.N(); i++)
      {
        VertexIndex index = f.LoadShort();
        poly.Set(i,index);
      }
      Offset offset = _face.Add(poly);
      // calculate old offset value from the new one
      OffsetAdvance(offset,i*OldPolyOverhead);
      oconv->Add(offset);
    }
    Offset end = _face.End();
    OffsetAdvance(end,_face.Size()*OldPolyOverhead);
    oconv->Add(end);
    // context during loading of old binarized files is Offset->Offset conversion
    f.SetContext(oconv);
  }
  else
  {
    DoAssert(version>7);
    if (version >= 12)
    {
      // skip the stripization id - stripization not used
      f.LoadShort();
    }
    _face.ReserveRaw(size);
    // calculate size in the file
    // each count is represented by 2 B in size, but only 1B in the file
    int faceDataSize = size-count;
    f.PreReadSequential(faceDataSize);
    for (int i=0; i<count; i++)
    {
      Poly poly;
      // reset IsAnimated / IsAlpha flags based on texture props
      poly.SetN(f.LoadChar());
      for (int i=0; i<poly.N(); i++)
      {
        VertexIndex index = f.LoadShort();
        poly.Set(i,index);
      }
      _face.Add(poly);
      // calculate old offset value from the new one
    }
  }
}

bool Shape::RequestFaces(const QFBankHandle &source, LODShape *lodShape, int version
#ifdef _M_PPC
 , QFileEndian endian
#endif
 , bool lzoCompression)
{
  QIFStreamB fs;
  fs.open(source);
  // handle stores exact limits for the face array
  return fs.PreRead(FileRequestPriority(10));
}

void Shape::LoadFaces(const QFBankHandle &source, LODShape *lodShape, int version
#ifdef _M_PPC
, QFileEndian endian
#endif
, bool lzoCompression)
{
  if (_face._data)
  {
    // already loaded - return
    return;
  }
  QIFStreamB fs;
  fs.open(source);

  SerializeBinStream f(&fs);
  f.SetVersion(version);
  #ifdef _M_PPC
  f.SetEndianState(endian);
  #endif
  if (lzoCompression) f.UseLZOCompression();
  
  DoFaceLoad(f,lodShape,version);
}

void Shape::UnloadFaces()
{
  //Assert(_source.NotNull());
  _face._data.Free();
}

size_t Shape::FacesSize() const
{
  Assert(IsLoadLocked());
  return _face._data ? _face._data->_faces.RawSize() : 0;
}

RString ShapeLoadSourceQFStreamB::GetDebugName() const
{
  return _handle.GetDebugName();
}

size_t VertexTableData::GetMemoryAllocated() const
{
  size_t sum = sizeof(*this);
  sum += _pos.GetMemoryAllocated();
  sum += _norm.GetMemoryAllocated();
  sum += _st.GetMemoryAllocated();
  sum += _alpha.GetMemoryAllocated();
  sum += _tex.GetMemoryAllocated();
  for (int i = 0; i < lenof(_texCoord); i++) sum += _texCoord[i - 1].GetMemoryAllocated();
  sum += _vertexBoneRef.GetMemoryAllocated();
  sum += _neighborBoneRef.GetMemoryAllocated();
  sum += _clip.GetMemoryAllocated();
  return sum;
}

size_t VertexTable::GetMemoryControlledByShape(bool includeUnloadable) const
{
  size_t sum = 0;

  if (includeUnloadable)
  {
    if (_data)
    {
      sum += _data->GetMemoryAllocated();
    }
  }
  return sum+sizeof(*this);
}
void Shape::DisableGeometryStreaming(ShapeRef *shapeRef)
{
  if (!_source) return;
  LoadGeometry(shapeRef);
  ResetLoadHandle();
}

void Shape::ResetLoadHandle()
{
  _source = NULL;
  _loadCount = 0;
}

void Shape::SetLoadHandle(
  const QFBankHandle &handle, const QFBankHandle &handleFaces,
  LODShape *lShape, int version
#ifdef _M_PPC
  , QFileEndian littleEndian
#endif
  , bool lzoCompression)
{
  if (NVertex()>0)
  {
    _source = new ShapeLoadSourceQFStreamB(handle,handleFaces,lShape,version
#ifdef _M_PPC
      , littleEndian ? EndianLittle : EndianBig
#endif
      , lzoCompression);
  }
  else
  {
    // empty shape can not be unloaded (it makes no sense anyway)
    _source = NULL;
    _loadCount = 0;
  }
}

bool Shape::IsGeometryLoaded() const
{
  Assert(_data.IsNull() || _data->_pos.Size()==_nVertex);
  return _nVertex==0 || _data.NotNull();
}

bool Shape::RequestGeometry() const
{
  if (!_source)
  {
    RptF("Cannot request geometry - no source (%d vertices, %d loaded)",NVertex(),NPos());
    Fail("Cannot request geometry");
    return true;
  }

  bool ret = _source->Request(unconst_cast(this));
  /*
  if (ret)
  {
    // if request was successful, we may immediately load it
  }
  */
  return ret;
}

void Shape::LoadGeometry(const ShapeRef *shapeRef)
{
  // Load geometry and animation data
  if (_source)
  {
    _source->Load(this);
    if (shapeRef)
    {
      _loadCount++;
      shapeRef->OnGeometryLoaded(this);
      _loadCount--;
}
  }
}

void Shape::UnloadGeometry()
{
  if (!GeometryUnloadedHandler())
  {
    return;
  }
#if _ENABLE_PHYSICS
	if(_physGeoms)// && GetNetworkManager().IsServerOwner())
  {
    for(int i = 0; i < _physGeomsLen; ++i)
    {
      delete (btCollisionShape*)(_physGeoms[i].m_pGeom);
    }
    
    GMemFunctions->Delete(_physGeoms);
    _physGeoms = 0;
  }
   
  _physGeomsLen = 0;
#endif
  UnloadVertices();
  UnloadFaces();
}


RString Shape::GetDebugName() const
{
  if (!_source)
  {
    return "???";
  }
  return _source->GetDebugName();
}


bool VertexTable::RequestVertices(const QFBankHandle &source, LODShape *lodShape, int version
#ifdef _M_PPC
,QFileEndian littleEndian
#endif
, bool lzoCompression)
{
  if (_data)
  {
    Fail("Geometry already loaded");
    return true;
  }
  QIFStreamB fs;
  fs.open(source);
  // handle stores exact limits for the geometry data
  return fs.PreRead(FileRequestPriority(10));
}

void VertexTable::GeometryLoadedHandler()
{
}

bool VertexTable::GeometryUnloadedHandler()
{
  return true;
}

/// serialize normals in uncompressed form
static void TransferNormals(SerializeBinStream &f, CondensedAutoArray<Vector3Compressed> &compressed)
{
  // conversion to CondensedAutoArray<Vector3>
  CondensedAutoArray<Vector3> norm;
  if (f.IsSaving())
  {
    if (compressed.IsCondensed())
    {
      norm.SetCondensed(compressed.GetCondensedValue().Get(), compressed.Size());
    }
    else
    {
      int n = compressed.Size();
      norm.Realloc(n);
      norm.Resize(n);
      for (int i=0; i<n; i++) norm.Set(i) = compressed[i].Get();
    }
  }

  // serialization
  f.TransferBinaryCondensedArray(norm, true);

  // conversion from CondensedAutoArray<Vector3>
  if (f.IsLoading())
  {
    if (norm.IsCondensed())
    {
      Vector3Compressed v(norm.GetCondensedValue());
      compressed.SetCondensed(v, norm.Size());
    }
    else
    {
      int n = norm.Size();
      compressed.Realloc(n);
      compressed.Resize(n);
      for (int i=0; i<n; i++) compressed.Set(i).Set(norm[i]);
    }
  }
}

struct STPairUncompressed
{
  Vector3 s, t;
};
TypeIsSimple(STPairUncompressed);

template <> struct EndianTraits<STPairUncompressed>: public EndianTraitsSwap<float> {};

/// serialize ST in uncompressed form
static void TransferST(SerializeBinStream &f, AutoArray<STPair> &compressed)
{
  AutoArray<STPairUncompressed> st;

  // conversion to uncompressed format
  if (f.IsSaving())
  {
    int n = compressed.Size();
    st.Realloc(n);
    st.Resize(n);
    for (int i=0; i<n; i++)
    {
      st[i].s = compressed[i].s.Get();
      st[i].t = compressed[i].t.Get();
    }
  }

  // serialization
  f.TransferBinaryArray(st);

  // conversion from uncompressed format
  if (f.IsLoading())
  {
    int n = st.Size();
    compressed.Realloc(n);
    compressed.Resize(n);
    for (int i=0; i<n; i++)
    {
      compressed[i].s.Set(st[i].s);
      compressed[i].t.Set(st[i].t);
    }
  }
}

/// serialize UV in uncompressed form
static void TransferUV(SerializeBinStream &f, UVPairs &compressed)
{
  // conversion to CondensedAutoArray<UVPair>
  CondensedAutoArray<UVPair> uv;
  if (f.IsSaving())
  {
    int n = compressed.Size();
    if (n == 0)
    {
      uv.SetCondensed(UVPair(0, 0), 0);
    }
    else if (compressed.IsCondensed())
    {
      uv.SetCondensed(compressed.UV(0), n);
    }
    else
    {
      uv.Realloc(n);
      uv.Resize(n);
      for (int i=0; i<n; i++) uv.Set(i) = compressed.UV(i);
    }
  }

  // serialization
  f.TransferBinaryCondensedArray(uv, true);

  // conversion from CondensedAutoArray<Vector3>
  if (f.IsLoading())
  {
    int n = uv.Size();
    if (n == 0)
    {
      compressed.SetCondensed(UVPair(0, 0), 0);
    }
    else if (uv.IsCondensed())
    {
      compressed.SetCondensed(uv.GetCondensedValue(), n);
    }
    else
    {
      compressed.Realloc(n);
      compressed.Resize(n);

      // find UV range
      UVPair minUV, maxUV, invUV;
      minUV.u = FLT_MAX;
      minUV.v = FLT_MAX;
      maxUV.u = -FLT_MAX;
      maxUV.v = -FLT_MAX;
      for (int i=0; i<n; i++)
      {
        if (uv[i].u > maxUV.u) maxUV.u = uv[i].u;
        if (uv[i].v > maxUV.v) maxUV.v = uv[i].v;
        if (uv[i].u < minUV.u) minUV.u = uv[i].u;
        if (uv[i].v < minUV.v) minUV.v = uv[i].v;
      }
      compressed.InitUV(invUV, minUV, maxUV);

      // set uv
      for (int i=0; i<n; i++)
      {
        compressed.SetUV(i, invUV, uv[i].u, uv[i].v);
      }
    }
  }
}

struct ChangeSignVectorCompressed
{
  bool operator () (Vector3Compressed &vc) const
  {
    // decompress, change sign, compress
    vc.Set(-vc.Get());
    return false;
  }
  bool operator () (STPair &st) const
  {
    // let XM library assert as needed
    st.s.Set(-st.s.Get());
    st.t.Set(-st.t.Get());
    return false;
  }
};

bool VertexTable::LoadVertices(
  const QFBankHandle &source, LODShape *lodShape, int version
#ifdef _M_PPC
  , QFileEndian endian
#endif
  , bool lzoCompression
)
{
  if (_data)
  {
    // if it is already loaded - do not load again
    // point/vertex count may differ as a result of post-load processing
    Assert(_data->_pos.Size()==_nVertex);
    Assert(_data->_tex.Size()==_data->_pos.Size());
    for (int i = 1; i < NUVStages(); i++)
    {
      Assert(_data->_texCoord[i - 1].Size()==_data->_pos.Size());
    }
    Assert(_data->_norm.Size()==_data->_pos.Size());
    Assert(_data->_clip.Size()==_data->_pos.Size());
    Assert(_data->_st.Size() == _data->_pos.Size() || _data->_st.Size() == 0);
    Assert(_data->_vertexBoneRef.Size() == _data->_pos.Size() || _data->_vertexBoneRef.Size() == 0);
    Assert(_data->_neighborBoneRef.Size() == _data->_pos.Size() || _data->_neighborBoneRef.Size() == 0);
    return false;
  }
  // load geometry, but nothing more
  QIFStreamB fs;
  fs.open(source);
  SerializeBinStream f(&fs);
  f.SetVersion(version);
  #if _M_PPC
  f.SetEndianState(endian);
  #endif
  if (lzoCompression) f.UseLZOCompression();

  f.LoadInt(); // skip end geom
  InitVertexTableData();
  if (f.GetVersion() >= 50)
  {
    f.TransferBinaryCondensedArray(_data->_clip, true);
  }
  else
  {
    if (_clipOldFormat)
    {
      _data->_clip = *_clipOldFormat;
    }
    else
    {
      // missing clip data, use heuristics only: assume clip is condensed, therefore ClipAnd and ClipOr define it well
      _data->_clip.SetCondensed(_andHints,_nVertex);
      // if this is not satisfied, warn
      if (_andHints!=_orHints)
      {
        // when there is no report stack, it is because we are not really streaming
        // the same function is used during disabling streaming
        if (CHECKREPORTSTACK())
        {
          RptF("Vertex flags used in %s with streaming - rebinarize needed",GETREPORTSTACK());
        }
      }
    }
  }
  if (f.GetVersion() >= 45)
  {
    // serialize in native format
    _data->_tex.SerializeBin(f);
    // this function is loading only
    int nUVStages = f.LoadInt();
    DoAssert((nUVStages > 0) && (nUVStages <= lenof(_data->_texCoord)+1));
    for (int i=1;  i<nUVStages; i++)
    {
      _data->_texCoord[i - 1].SerializeBin(f);
    }
    // make sure condensed representation does not contain NaN
    for (int i=nUVStages; i<lenof(_data->_texCoord)+1; i++)
    {
      _data->_texCoord[i-1].SetCondensed(UVPair(0, 0), 0);
    }
  }
  else
  {
    TransferUV(f, _data->_tex);
    if (f.GetVersion() >= 35)
    {
      // this function is loading only
      int nUVStages = f.LoadInt();
      DoAssert((nUVStages > 0) && (nUVStages <= lenof(_data->_texCoord)+1));
      for (int i = 1;  i < nUVStages; i++)
      {
        TransferUV(f, _data->_texCoord[i - 1]);
      }
      // make sure condensed representation does not contain NaN
      for (int i=nUVStages; i<lenof(_data->_texCoord)+1; i++)
      {
        _data->_texCoord[i-1].SetCondensed(UVPair(0, 0), 0);
      }
    }
  }
  f.TransferBinaryArray(_data->_pos, true);
  if (f.GetVersion() >= 45)
  {
    // serialize in native format
    f.TransferBinaryCondensedArray(_data->_norm, true);
    f.TransferBinaryArray(_data->_st, true);

    if (f.IsLoading() && f.GetVersion() == 45)
    {
      // in version 45, normals was not stored correctly (-normal instead of normal)
      _data->_norm.ForEach(ChangeSignVectorCompressed());
      _data->_st.ForEachF(ChangeSignVectorCompressed());
    }
  }
  else
  {
    TransferNormals(f, _data->_norm);
    if (f.GetVersion() >= 29) TransferST(f, _data->_st);
  }
  f.TransferBinaryArray(_data->_vertexBoneRef);
  f.TransferBinaryArray(_data->_neighborBoneRef);
#if _M_PPC
  // endian processing for _vertexBoneRef (AnimationRTWeight) and _neighborBoneRef (VertexNeighborInfo) is not simple
  // the structure is not homogeneous
  for (int i=0; i<_data->_vertexBoneRef.Size(); i++)
  {
    AnimationRTWeight &info = _data->_vertexBoneRef[i];
    // TODOX360: implement swap inside of VerySmallArray instead of direct memory access
    f.ProcessLittleEndianData(&info,sizeof(int),sizeof(int));
  }
  for (int i=0; i<_data->_neighborBoneRef.Size(); i++)
  {
    VertexNeighborInfo &info = _data->_neighborBoneRef[i];
    // TODOX360: implement swap inside of VerySmallArray instead of direct memory access
    f.ProcessLittleEndianData(&info._posA,sizeof(info._posA),sizeof(info._posA));
    f.ProcessLittleEndianData(&info._rtwA,sizeof(int),sizeof(int));
    f.ProcessLittleEndianData(&info._posB,sizeof(info._posB),sizeof(info._posB));
    f.ProcessLittleEndianData(&info._rtwB,sizeof(int),sizeof(int));
  }
#endif

  // revert back what could be changed by post-load processing
  _nVertex = _data->_pos.Size();
  DoAssert(_nVertex < 32768);
  _data->_clip.Resize(_nVertex);

  Assert(_data->_tex.Size()==_nVertex);
  for (int i = 1; i < NUVStages(); i++)
  {
    Assert(_data->_texCoord[i - 1].Size()==_nVertex);
  }
  Assert(_data->_norm.Size()==_nVertex);
  Assert(_data->_clip.Size()==_nVertex);
  Assert(_data->_st.Size() == _nVertex || _data->_st.Size() == 0);
  Assert(_data->_vertexBoneRef.Size() == _nVertex || _data->_vertexBoneRef.Size() == 0);
  Assert(_data->_neighborBoneRef.Size() == _nVertex || _data->_neighborBoneRef.Size() == 0);

  if (_nVertex==0) _data.Free();
  if (_nVertex>0)
  {
    int n = _nVertex;
    int rem = lodShape->Remarks();
    if (rem&ShapeReversed)
    {
      for (int i=0; i<n; i++)
      {
        ReverseVector(_data->_pos[i]);
      }
      _data->_norm.ForEach(ReverseVectorCompressed);
      if (IsSTPresent())
      {
        for (int i=0; i<n; i++)
        {
          ReverseVectorCompressed(_data->_st[i].s);
          ReverseVectorCompressed(_data->_st[i].t);
        }
      }
    }
    //Vector3 origToPos = _orig[0]-_pos[0];

    // apply autocenter to all positions]
    Vector3Val bcenter = lodShape->BoundingCenter()-lodShape->BoundingCenterOnLoad();
    if (bcenter.SquareSize()>0)
    {
      for (int i=0; i<n; i++)
      {
        _data->_pos.Set(i) -= bcenter;
      }
    }
  }
  // call post-load geometry processing
  GeometryLoadedHandler();

  return true;
}
void VertexTable::UnloadVertices()
{
  _data.Free();
}

class MaterialList: public RefArray<TexMaterial>
{
  public:
  int FindMaterial(const char *name) const;
  void AddMaterial(TexMaterial *mat)
  {
    int index = FindMaterial(mat->GetName());
    if (index<0) Add(mat);
  }
};

int MaterialList::FindMaterial(const char *name) const
{
  for (int i=0; i<Size(); i++)
  {
    if (!strcmpi(Get(i)->GetName(),name)) return i;
  }
  return -1;
}
  

void ShapeSection::SerializeBin(SerializeBinStream &f, Shape *shape, const MaterialList &materials)
{
  f.TransferLittleEndian(beg);
  f.TransferLittleEndian(end);
  f.Transfer(_minBoneIndex);
  f.Transfer(_bonesCount);
  // TODO: remove from file
  int materialDummy = 0;
  f.Transfer(materialDummy);
  // transfer poly properties
  // use Shape to get texture index
  if (f.IsSaving())
  {
    Texture *tex = GetTexture();
    int index = tex ? shape->GetTextureIndex(tex) : -1;
    f.SaveShort(index);
    f.SaveInt(Special());
    // save material name
    RString mat = _surfMat ? RString(cc_cast(_surfMat->GetName())) : RString();
    int matIndex = materials.FindMaterial(mat);
    f.SaveInt(matIndex);
    if (matIndex<0)
    {
      // material not embedded
      f.Transfer(mat);
    }
    if (f.GetVersion() >= 36)
    {
      f.SaveInt(UV_STAGES);
      for (int i = 0; i < UV_STAGES; i++)
      {
        float aot = _areaOverTex.Get(i);
        f.Transfer(aot);
      }
    }
    else
    {
      float aot = _areaOverTex.Get(0);
      f.Transfer(aot);
    }
  }
  else
  {
    if (materialDummy)
    {
      RptF("Old style material %d used in %s",materialDummy,GETREPORTSTACK());
    }
    int index = f.LoadShort();
    Texture *texture = index>=0 ? shape->GetTexture(index) : NULL;
    SetTexture(texture);
    SetSpecial(f.LoadInt());
    int matIndex = -1;
    // materials are loaded from a material index
    matIndex = f.LoadInt();
    if (matIndex>=0 && matIndex<materials.Size())
    {
      _surfMat = materials[matIndex];
    }
    else
    {
      RString mat;
      f.Transfer(mat);
      if (mat.GetLength()>0)
      {
        mat.Lower();
        _surfMat = GTexMaterialBank.New(TexMaterialName(mat));
      }
      else
      {
        _surfMat = NULL;
      }
    }
    #if _DEBUG
    if (!_surfMat)
    {
      // any material from CfgTextureToMaterial should already be processed during binarization
      _surfMat = GTexMaterialBank.TextureToMaterial(texture);
      Assert(!_surfMat);
    }
    #endif

    if (f.GetVersion() >= 36)
    {
      int nStages = f.LoadInt();
      DoAssert(UV_STAGES == nStages);
      float aot;
      for (int i = 0; i < nStages; i++)
      {
        f.Transfer(aot);
        _areaOverTex.Set(i, aot);
      }
    }
    else
    {
      float aot;
      f.Transfer(aot);
      _areaOverTex.Set(0, aot);
    }

    #if 0
    // experimental hack - prevent UV mapping which exceeds reasonable limits - news:g8mgb4$v5r$1@new-server.localdomain
    for (int i=0; i<UV_STAGES; i++)
    {
     if (_areaOverTex.Get(i)<0)
     {
       // uninitialized value - should be unused
       #if !_DEBUG
       // to be safe we replace with a very conservative value
       // but not in debug - in debug we have asserts for negative value
       _areaOverTex.Set(i,1e-6f);
       #endif
       continue;
     }
     // problematic is: large texel
     // this means big ration texel/area, small ratio area/texel
     const float maxTexel = 0.01f;
     // we need to adjust in respect to some particular texture size
     // for start we assume texture size 1024 to keep this simple
     const float texSize = 1024;
     // float texel = sqrt(_areaOverTex.Get(i)) / texSize;
     // if (texel>maxTexel)
     if (_areaOverTex.Get(i)>Square(maxTexel*texSize)) // avoid division and sqrt
     {
       // maxTexel =  sqrt(_areaOverTex[i])/texSize;
       _areaOverTex.Set(i,Square(maxTexel*texSize));
     }
    }

    #endif
  }
}

DEF_RSB(keyframe);

#define LOG_SHAPE_LOADING 0

/*!
\param f serialization stream
\param loadGeometry when false, vertices and faces are not stored, only skipped
\patch_internal 1.31 Date 11/23/2001 by Ondra.
- Fixed: Shared proxy objects (like building interior equipment)
are now indestructible.
*/

struct ChangeSignVectorCompressed2
{
  bool operator () (Vector3Compressed &vc) const
  {
    // decompress, change sign, compress
    vc.Set(-vc.Get());
    return false;
  }
  bool operator () (STPair &st) const
  {
    // let XM library assert as needed
    st.s.Set(-st.s.Get());
    st.t.Set(-st.t.Get());
    return false;
  }
};

struct EmptyDataGuard
{
  Shape *_shape;
  EmptyDataGuard(Shape *shape, bool isSaving) : _shape(NULL)
  {
    if (isSaving && !shape->GetVertexTableData())
    {
      shape->InitVertexTableData();
      _shape = shape;
    }
  }
  ~EmptyDataGuard()
  {
    if (_shape) _shape->FreeVertexTableData();
  }
};

void Shape::SerializeBin(
  SerializeBinStream &f,bool loadGeometry,
  int &begGeom, int &endGeom, int &begFaces, int &endFaces, LODShape *lodShape
)
{
#if LOG_SHAPE_LOADING
  int start = f.TellG();
#endif

  EmptyDataGuard emptyDataGuard(this, f.IsSaving()); //when saving, possible NULL _data is reinitialized to avoid crash

  // Following code must be the first in this function ---------------------------
  
  // first of all save information about other files we depend on
  f.TransferRefArray(_proxy);

#if LOG_SHAPE_LOADING
  LogF(" - after proxies - offset %d", f.TellG() - start);
#endif

  // -----------------------------------------------------------------------------

  // Serialize the subskeleton arrays
  f.TransferBasicArray(_subSkeletonToSkeleton);
  f.TransferBasicArray(_skeletonToSubSkeleton);
  
  if (f.IsLoading())
  {
    // TODO: create a new binarized version which will already store the subskeleton hierarchy
    if (lodShape->GetSkeletonSource())
    {
      CreateSubskeletonHierarchy(lodShape->GetSkeletonSource());
    }
  }

#if LOG_SHAPE_LOADING
  LogF(" - after skeletons - offset %d", f.TellG() - start);
#endif
  
  if (f.IsSaving())
  {
    // make sure all which can be condensed is condensed
    _data->_clip.Condense();
    _data->_tex.Condense();
    for (int i = 0; i < NUVStages(); i++) _data->_texCoord[i - 1].Condense();
    _data->_norm.Condense();
  }
  // geometry needs to be locked during serialization
  _loadWarning = false;
  if (f.GetVersion()<50)
  {
    if (f.IsLoading())
    {
      _clipOldFormat = new CondensedAutoArray<ClipFlags>();
      f.TransferBinaryCondensedArray(*_clipOldFormat, true);
      _nVertex = _clipOldFormat->Size();
    }
    else // we want to be able to handle format 48 saving as well
    {
      f.TransferBinaryCondensedArray(_data->_clip, true);
    }
  }
  else
  {
    f.Transfer(_nVertex);
  }
  DoAssert(_nVertex < 32768);
  if (f.GetVersion()>=51)
  {
    f.Transfer(_faceArea);
  }
  else
  {
    if (f.IsLoading())
      _faceArea = FLT_SNAN;
  }
  
  f.TransferLittleEndian(_orHints);
  f.TransferLittleEndian(_andHints);

  f.Transfer(_minMax[0]);f.Transfer(_minMax[1]);
  f.Transfer(_bCenter);
  f.Transfer(_bRadius);

#if LOG_SHAPE_LOADING
  LogF(" - after hints - offset %d", f.TellG() - start);
#endif

  MaterialList materials;
  if (f.IsLoading())
  {
    _allTexturesReady = false;

    int size = f.LoadInt();
    _textures.Realloc(size);
    _textures.Resize(size);
    for (int i=0; i<_textures.Size(); i++)
    {
      RString str;
      f.Transfer(str);
      if (str.GetLength()>0)
      {
        _textures[i]=GlobLoadTexture(str);
      }
      else
      {
        _textures[i]=NULL;
      }
    }
    size = f.LoadInt();
    materials.Realloc(size);
    materials.Resize(size);
    for (int i=0; i<materials.Size(); i++)
    {
      // transfer material name
      RString name;
      f.Transfer(name);
      // transfer material content
      materials[i] = GTexMaterialBank.NewEmbedded(name,f,lodShape->GetName());
    }
  }
  else
  {
    f.SaveInt(_textures.Size());
    for (int i=0; i<_textures.Size(); i++)
    {
      Texture *txt = _textures[i];
      RString txtName = txt ? txt->GetName() : "";
      f.Transfer(txtName);
    }
    // scan all sections to get all materials
    for (int i=0; i<NSections(); i++)
    {
      const ShapeSection &sec = GetSection(i);
      TexMaterial *mat = sec.GetMaterialExt();
      if ( mat && mat->GetName()[0])
      {
        materials.AddMaterial(mat);
      }
    }
    // save all materials
    f.SaveInt(materials.Size());
    for (int i=0; i<materials.Size(); i++)
    {
      TexMaterial *mat = materials[i];
      RString name = mat->GetName();
      f.Transfer(name);
      mat->SerializeBin(f);
    }
  }
  f.TransferBinaryArray(_pointToVertex); // are associated with texture
  f.TransferBinaryArray(_vertexToPoint); // are associated with texture

#if LOG_SHAPE_LOADING
  LogF(" - after materials - offset %d", f.TellG() - start);
#endif

  int spec = 0;
  if (f.IsLoading())
  {
    // TODO: when not loading geometry, skip faces instead of loading them
    begFaces = f.TellG();
    
    
    int count = f.LoadInt();
    int size = f.LoadInt();
    // skip the stripization ID 
    f.LoadShort();
    // load data to temporary buffer
    // during face loading create offset conversion table
    _face._data = new FaceData;
    _face.ReserveRaw(size);
    // calculate size in the file
    // each count is represented by 2 B in size, but only 1B in the file
    int faceDataSize = size-count;
    f.PreReadSequential(faceDataSize);
    for (int i=0; i<count; i++)
    {
      Poly poly;
      // reset IsAnimated / IsAlpha flags based on texture props
      poly.SetN(f.LoadChar());
      for (int i=0; i<poly.N(); i++)
      {
        VertexIndex index = f.LoadShort();
        poly.Set(i,index);
      }
      // TODO: if (!loadGeometry)
      _face.Add(poly);
      // calculate old offset value from the new one
    }
    endFaces = f.TellG();
    if (!loadGeometry)
    {
      _face._data.Free();
    }
  }
  else
  {
    begFaces = f.TellP();
    // currently no save binarized save is supported
    // faces must be saved one by one
    // and texture pointers converted to texture indices
    f.SaveInt(_face.Size());
    f.SaveInt(_face.RawSize());
    // stripization ID - no longer used
    f.SaveShort(0);
    for (Offset o=_face.Begin(); o<_face.End(); _face.Next(o))
    {
      const Poly &face = _face[o];

      //f.SaveInt(face.Special());
      //f.SaveShort(_textures.Find(face.GetTexture()));
      f.SaveChar(face.N());
      f.Save(face.GetVertexList(),face.N()*sizeof(VertexIndex));
    }
    endFaces = f.TellP();
  }

  // transfer shape sections
  // cannot use f.TranserArray, because we need to pass this
  if (f.IsLoading())
  {
    int size = f.LoadInt();
    _face._sections.Realloc(size);
    _face._sections.Resize(size);
    for (int i=0; i<size; i++)
    {
      _face._sections[i].SerializeBin(f,this,materials);
    }
    // following assert cannot be done when not loading geometry
    DoAssert
    (
      !loadGeometry ||
      _face._sections.Size()==0 ||
      _face._sections[_face._sections.Size()-1].end==EndFaces()
    );
    #if _ENABLE_REPORT
    // assert sections are well ordered
    for (int i=1; i<size; i++)
    {
      int flags0 = _face._sections[i-1].Special();
      int flags1 = _face._sections[i].Special();
      bool alpha0 = (flags0&IsAlphaOrdered)!=0;
      bool alpha1 = (flags1&IsAlphaOrdered)!=0;
      if (alpha1<alpha0)
      {
        RptF
        (
          "  bad section order: %s before %s",
          (const char *)_face._sections[i-1].GetDebugText(),
          (const char *)_face._sections[i].GetDebugText()
        );
        _loadWarning = true;
      }
    }
    #endif
    // Set the vertex declaration according to sections
    VertexDeclarationNeedsUpdate();
    SetCanBeInstanced();
    // textures loaded - we may adjust them now
    AdjustTextures();
  }
  else
  {
    f.SaveInt(_face._sections.Size());
    for (int i=0; i<_face._sections.Size(); i++)
    {
      _face._sections[i].SerializeBin(f,this,materials);
    }
  }

  //Assert(CheckIntegrity());
  f.TransferArray(_sel);

#if LOG_SHAPE_LOADING
  LogF(" - after selections - offset %d", f.TellG() - start);
#endif

  //Assert(CheckIntegrity());

  f.TransferArray(_prop);
  if (f.IsSaving() && _phase.Size()>0)
  {
    
    bool phaseEnabled = atoi(PropertyValue(RSB(keyframe)))!=0;
    if (!phaseEnabled)
    {
      // keyframe animation not enabled - delete it
      _phase.Clear();
    }
  }
  f.TransferArray(_phase);
  if (f.IsLoading() && _phase.Size()>0)
  {
    // check if keyframe animation is enabled
    bool phaseEnabled = atoi(PropertyValue(RSB(keyframe)))!=0;
    if (!phaseEnabled)
    {
      RptF("Keyframe animation, no keyframe property");
      LogF("  memory used %d",_phase.Size()*_phase[0].Size()*sizeof(Vector3));
      _phase.Clear();
      _loadWarning = true;
    }
  }

  f.TransferLittleEndian(_colorTop);
  f.TransferLittleEndian(_color);
  f.Transfer(_special);

  if (f.IsLoading())
  {
    _special |= spec;
    // reinit mutable/temporary members
    _faceNormalsValid = true;
  }
  

  f.Transfer(_vertexBoneRefIsSimple);

#if LOG_SHAPE_LOADING
  LogF(" - prior geometry - offset %d", f.TellG() - start);
#endif

  begGeom = f.IsLoading() ? f.TellG() : f.TellP();
  int endGeomFixup = 0;
  if (f.IsLoading())
  {
    endGeom = f.LoadInt() + begGeom;
  }
  else
  {
    endGeomFixup = f.CreateFixUp();
  }
  if (!loadGeometry)
  {
    if (f.IsLoading()) f.SeekG(endGeom);
  }
  else
  {
    InitVertexTableData();
    if (f.IsLoading())
    {
      f.PreReadSequential(endGeom-begGeom);
    }
    if (f.GetVersion() >= 50)
    {
      f.TransferBinaryCondensedArray(_data->_clip, true);
    }
    else if (f.IsLoading())
    {
      _data->_clip = *_clipOldFormat;
    }
    if (f.GetVersion() >= 45)
    {
      // serialize in native format
      _data->_tex.SerializeBin(f);
      int nUVStages;
      if (f.IsLoading())
      {
        nUVStages = f.LoadInt();
      }
      else
      {
        nUVStages = NUVStages();
        f.SaveInt(nUVStages);
      }
      DoAssert((nUVStages > 0) && (nUVStages <= lenof(_data->_texCoord)+1));
      for (int i=1; i<nUVStages; i++)
      {
        _data->_texCoord[i - 1].SerializeBin(f);
      }
    }
    else
    {
      TransferUV(f, _data->_tex);
      if (f.GetVersion() >= 35)
      {
        int nUVStages;
        if (f.IsLoading())
        {
          nUVStages = f.LoadInt();
        }
        else
        {
          nUVStages = NUVStages();
          f.SaveInt(nUVStages);
        }
        DoAssert((nUVStages > 0) && (nUVStages <= lenof(_data->_texCoord)+1));
        for (int i = 1;  i < nUVStages; i++)
        {
          TransferUV(f, _data->_texCoord[i - 1]);
        }
      }
    }
#if LOG_SHAPE_LOADING
    LogF(" - after UV - offset %d", f.TellG() - start);
#endif

    f.TransferBinaryArray(_data->_pos,true);
#if LOG_SHAPE_LOADING
    LogF(" - after positions - offset %d", f.TellG() - start);
#endif

    if (f.GetVersion() >= 45)
    {
      // serialize in native format
      f.TransferBinaryCondensedArray(_data->_norm, true);
      f.TransferBinaryArray(_data->_st, true);
      if (f.IsLoading() && f.GetVersion() == 45)
      {
        // in version 45, normals was not stored correctly (-normal instead of normal)
        _data->_norm.ForEach(ChangeSignVectorCompressed2());
        _data->_st.ForEachF(ChangeSignVectorCompressed2());
      }
    }
    else
    {
      TransferNormals(f, _data->_norm);
      if (f.GetVersion() >= 29) TransferST(f, _data->_st);
    }
#if LOG_SHAPE_LOADING
    LogF(" - after normals & ST - offset %d", f.TellG() - start);
#endif

    f.TransferBinaryArray(_data->_vertexBoneRef); 
    f.TransferBinaryArray(_data->_neighborBoneRef);
    #if _M_PPC
      // endian processing for _vertexBoneRef (AnimationRTWeight) and _neighborBoneRef (VertexNeighborInfo) is not simple
      // the structure is not homogeneous
      for (int i=0; i<_data->_vertexBoneRef.Size(); i++)
      {
        AnimationRTWeight &info = _data->_vertexBoneRef[i];
        // TODOX360: implement swap inside of VerySmallArray instead of direct memory access
        f.ProcessLittleEndianData(&info,sizeof(int),sizeof(int));
      }
      for (int i=0; i<_data->_neighborBoneRef.Size(); i++)
      {
        VertexNeighborInfo &info = _data->_neighborBoneRef[i];
        // TODOX360: implement swap inside of VerySmallArray instead of direct memory access
        f.ProcessLittleEndianData(&info._posA,sizeof(info._posA),sizeof(info._posA));
        f.ProcessLittleEndianData(&info._rtwA,sizeof(int),sizeof(int));
        f.ProcessLittleEndianData(&info._posB,sizeof(info._posB),sizeof(info._posB));
        f.ProcessLittleEndianData(&info._rtwB,sizeof(int),sizeof(int));
      }
    #endif
    
    Assert(_nVertex==_data->_norm.Size());
    Assert(_nVertex==_data->_tex.Size());
    for (int i = 0; i < NUVStages(); i++)
    {
      Assert(_nVertex==_data->_texCoord[i - 1].Size());
    }
    Assert(_nVertex==_data->_pos.Size());
    Assert(_nVertex == _data->_st.Size() || 0 == _data->_st.Size());
    Assert(_nVertex == _data->_vertexBoneRef.Size() || 0 == _data->_vertexBoneRef.Size());
    Assert(_nVertex == _data->_neighborBoneRef.Size() || 0 == _data->_neighborBoneRef.Size());
    if (f.IsLoading())
    {
      #if _DEBUG
        int checkEndGeom = f.TellG();
        Assert(checkEndGeom==endGeom);
      #endif
    }
    else
    {
      endGeom = f.TellP();
      f.FixUp(endGeomFixup,f.TellP()-begGeom);
    }
    
    if (f.IsLoading())
    {
      // check if TL can be used on the shape 
      if (!CustomTLRequired() && NFaces()!=0)
      {
        if (_orHints&ClipUserMask)
        {
          ClipAndAll(~ClipUserMask);
          _data->_clip.Condense();
        }
      }
      #if _ENABLE_REPORT
        if (!_data->_clip.IsCondensed() && _data->_clip.Size()>0)
        {
          Log(
            " not condensed, %x - %x : %x, size %d",
            _orHints,_andHints,_orHints^_andHints,_data->_clip.Size()
          );
          //_loadWarning = true;
        }
      #endif

    }

    if (_nVertex==0) _data.Free();
  }

  if (f.IsLoading())
  {
    OffsetToIndex *oconv = (OffsetToIndex *)f.GetContext();
    if (oconv)
    {
      delete oconv;
      f.SetContext(NULL);
    }
    Assert(CheckIntegrity());
  }
#if LOG_SHAPE_LOADING
  LogF(" - after geometry - offset %d", f.TellG() - start);
#endif
}

void LODShape::Reverse()
{
  // reverse all relevant vector data
  ReverseVector(_aimingCenter); // where should I lock/aim?

  for (int i=0; i<NLevels(); i++)
  {
    Shape *shape = InitLevelLocked(i);
    if (shape && shape->IsGeometryLoaded())
    {
      // we know we are loaded, pretend we are locked
      shape->AddRefLoad();
      shape->Reverse();
      shape->ReleaseLoad();
    }
  }
  if (_animations)
  {
    for (int i=0; i<_animations->Size(); i++)
    {
      (*_animations)[i]->Reverse(this);
    }
  }

  { 
    // components may be shared, we need to reverse each of them only once
    FindArrayKey<ConvexComponents *> components;
    components.AddUnique(_geomComponents);
    components.AddUnique(_fireComponents);
    components.AddUnique(_viewComponents);
    #if VIEW_INTERNAL_GEOMETRIES
      components.AddUnique(_viewPilotComponents);
      components.AddUnique(_viewGunnerComponents);
      components.AddUnique(_viewCargoComponents);
    #endif
    for (int i=0; i<components.Size(); i++) if (components[i]) for (int c=0; c<components[i]->Size(); c++)
    {
      (*components[i])[c]->Reverse();
    }
  }
    
  // note: min-max reversed
  ReverseMinMax(_minMax);
  ReverseMinMax(_minMaxVisual);
  // the point with smallest bounding sphere
  // we approximate it as (_min+_max)/2
  ReverseVector(_boundingCenter);
  ReverseVector(_boundingCenterOnLoad);
  ReverseVector(_geometryCenter);

  ReverseVector(_centerOfMass);
  // reverse corresponding axes of angular inertia tensor
  _invInertia(0,1) = -_invInertia(0,1);
  _invInertia(1,0) = -_invInertia(1,0);
  _invInertia(2,1) = -_invInertia(2,1);
  _invInertia(1,2) = -_invInertia(1,2);
  
  _inertia(0,1) = -_inertia(0,1);
  _inertia(1,0) = -_inertia(1,0);
  _inertia(2,1) = -_inertia(2,1);
  _inertia(1,2) = -_inertia(1,2);
}

void operator << (SerializeBinStream &f, SBoneRelation &br)
{
  f.Transfer(br._bone);
  f.Transfer(br._parent);
  #if _ENABLE_REPORT
    if  (!br._bone.IsLowCase())
    {
      RptF("Bone %s is not lowercase",cc_cast(br._bone));
    }
    if  (!br._parent.IsLowCase())
    {
      RptF("parent %s is not lowercase",cc_cast(br._parent));
    }
  #endif

}

#if SKELETONSEPARATETYPE
void operator << (SerializeBinStream &f, SkeletonIndex &si)
{
  if (f.IsLoading())
  {
    si = SetSkeletonIndex(f.LoadInt());
  }
  else
  {
    f.SaveInt(GetSkeletonIndex(si));
  }
}

void operator << (SerializeBinStream &f, SubSkeletonIndex &si)
{
  if (f.IsLoading())
  {
    int i = f.LoadInt();
    SetSubSkeletonIndex(si, i);
  }
  else
  {
    int i = GetSubSkeletonIndex(si);
    f.SaveInt(i);
  }
}
#endif

void operator << (SerializeBinStream &f, SubSkeletonIndexSet &si)
{
  f.TransferBasicArray(si);
}

void operator << (SerializeBinStream &f, Vector3Compressed &v)
{
  if (f.IsLoading())
  {
    DWORD val;
    f.TransferBinary(&val, sizeof(val));
    v.SetRaw(val);
  }
  else
  {
    DWORD val = v.GetRaw();
    f.TransferBinary(&val, sizeof(val));
  }
}

const char *LevelName(float resolution);

/** most properties were located at the end of the file in older version
This function enables moving their location easily */

void LODShape::SerializeBinHeaders(SerializeBinStream &f)
{
  // save common information

  f.Transfer(_special);
  f.Transfer(_boundingSphere);f.Transfer(_geometrySphere);

  f.Transfer(_remarks);
  f.TransferLittleEndian(_andHints);
  f.TransferLittleEndian(_orHints);

  f.Transfer(_aimingCenter);
  f.TransferLittleEndian(_color);
  f.TransferLittleEndian(_colorTop);
  f.Transfer(_viewDensity);
  f.Transfer(_minMax[0]);f.Transfer(_minMax[1]);

  if (f.GetVersion()>=52)
  {
    f.Transfer(_minMaxVisual[0]);f.Transfer(_minMaxVisual[1]);
  }
  else if (f.IsLoading())
  {
    _minMaxVisual[0] = _minMax[0];
    _minMaxVisual[1] = _minMax[1];
  }


  f.Transfer(_boundingCenter);f.Transfer(_geometryCenter);
  if (f.IsLoading())
  {
    _boundingCenterOnLoad = _boundingCenter;
  }

  f.Transfer(_centerOfMass);
  f.Transfer(_invInertia);
  
  f.Transfer(_autoCenter);f.Transfer(_lockAutoCenter);
  f.Transfer(_canOcclude);f.Transfer(_canBeOccluded);

  if ((f.GetVersion() >= 42 && f.GetVersion() < 10000) || (f.GetVersion() >= 10042))
  {
    f.Transfer(_htMin);
    f.Transfer(_htMax);
    f.Transfer(_afMax);
    f.Transfer(_mfMax);
//     LogF("*[TI] shape %s htMin %g",cc_cast(_name),_htMin);
//     LogF("*[TI] shape %s htMax %g",cc_cast(_name),_htMax);
//     LogF("*[TI] shape %s afMax %g",cc_cast(_name),_afMax);
//     LogF("*[TI] shape %s mfMax %g",cc_cast(_name),_mfMax);
  }
  else
  {
    if (f.IsLoading())
    {
      _htMin = 0.0f;//600.0f;
      _htMax = 0.0f;//3600.0f;
      _afMax = 0.0f;
      _mfMax = 0.0f;
//       LogF("*[TI] Zeroed htMin..mfMax for %s",cc_cast(_name));
    }
  }

  if ((f.GetVersion() >= 43 && f.GetVersion() < 10000) || (f.GetVersion() >= 10043))
  {
    f.Transfer(_mFact);
    f.Transfer(_tBody);
//     LogF("*[TI] shape %s mFact %g",cc_cast(_name),_mFact);
//     LogF("*[TI] shape %s tBody %g",cc_cast(_name),_tBody);
  }
  else
  {
    if (f.IsLoading())
    {
      _mFact = 0.0f;
      _tBody = 0.0f;
//       LogF("*[TI] Zeroed mFact..tBody for %s",cc_cast(_name));
    }
  }

  if (f.GetVersion() >= 33)
  {
    f.Transfer(_forceNotAlphaModel);
  }

  if (f.GetVersion() >= 37)
  {
    f.TransferLittleEndian(_sbSource);
    f.Transfer(_preferShadowVolume);
  }
  if (f.IsSaving())
  {
    // save the offset based on the property only, when there is no property, save FLT_MAX
    float shadowOffset = FLT_MAX;
    ReadShadowOffsetFromProperty(shadowOffset);
    f.Transfer(shadowOffset);
  }
  else
  {
    _shadowOffset = FLT_MAX;
    if (f.GetVersion()>=48)
    {
      f.Transfer(_shadowOffset);
    }
  }
  
  bool allowAnimation;
  if (_animationType == AnimTypeSoftware)
  {
    allowAnimation = true;
  }
  else
  {
    allowAnimation = false;
  }
  f.Transfer(allowAnimation);
  if (f.IsLoading())
  {
    if (allowAnimation)
    {
      if (_orHints&ClipLandKeep)
      {
        RptF("SW keep height animation used for %s",cc_cast(GetName()));
      }
      _animationType = AnimTypeSoftware;
    }
    else
    {
      _animationType = AnimTypeNone;
    }
  }

  RStringB skeletonName;
  if (f.IsLoading())
  {
    f.Transfer(skeletonName);
    if (skeletonName.GetLength() > 0)
    {
      _skeletonSource = SkeletonSources.NewEmbedded(skeletonName, f);
      _animationType = AnimTypeHardware;
    }
  }
  else
  {
    if (_skeletonSource.NotNull()) skeletonName = _skeletonSource->GetName();
    f.Transfer(skeletonName);
    if (skeletonName.GetLength() > 0) _skeletonSource->SerializeBin(f);
  }

  f.TransferLittleEndian(_mapType);

  if (f.IsLoading())
  {
    f.SkipBinaryArray(_massArray);
  }
  else
  {
    f.TransferBinaryArray(_massArray);
  }

  f.Transfer(_mass);
  f.Transfer(_invMass);

  f.Transfer(_armor);
  f.Transfer(_invArmor);
  if (f.IsLoading())
  {
    if (_armor>1e-10)
    {
      _logArmor = log(_armor);
    }
    else
    {
      _logArmor = 25;
    }

    if (_mass>0 && _invInertia!=M3Zero)
    {
      // to improve numerical stability use mass to make sure matrix is around 1
      _inertia = (_invInertia* _mass).InverseGeneral()* _mass;
      #if _DEBUG
        Matrix3 checkE = _inertia*_invInertia;
        Assert(checkE.Distance2(M3Identity)<Square(0.01f));
      #endif
    }
    else
    {
      // inertia has no sense for models without a mass
      _inertia = M3Identity;
    }
  }


  
  f.Transfer(_memory);
  f.Transfer(_geometry);
  f.Transfer(_geometryFire);
  f.Transfer(_geometryView);
  f.Transfer(_geometryViewPilot);
  f.Transfer(_geometryViewGunner);
  // TODO: removed geometryViewCommander - not used
  signed char geometryViewCommander = -1;
  f.Transfer(geometryViewCommander);
  #if _ENABLE_REPORT
    if (f.IsLoading())
    {
      if (geometryViewCommander>=0)
      {
        RptF("%s: Geometry view commander lod found - obsolete",cc_cast(GetName()));
      }
    }
  #endif
  f.Transfer(_geometryViewCargo);
  f.Transfer(_landContact);
  f.Transfer(_roadway);
  f.Transfer(_paths);
  f.Transfer(_hitpoints);

  // The LOD indexes must not be serialized. They are initialized by LODShape::ScanResolutions. That function 
  // is called during loading in LODShape::SerializeBin. TODO: Currently some of the LOD indexes are 
  // serialized some not... Clarify the status...

  //f.Transfer(_shadowVolume); 
  //f.Transfer(_subParts); 


  if (f.IsLoading())
  {
    _minShadow = f.LoadInt();
  }
  else
  {
    f.SaveInt(_minShadow);
  }

  if (f.GetVersion() >= 38)
  {
    f.Transfer(_canBlend);
  }

  f.Transfer(_propertyClass);
  f.Transfer(_propertyDamage);
  f.Transfer(_propertyFrequent);
  if (f.GetVersion()>=31)
  {
    #if _ENABLE_REPORT
    f.TransferBasicArray(_unusedSelectionNames);
    #else
    FindArrayRStringBCI dummy;
    f.TransferBasicArray(dummy);
    #endif
  }
  if (f.IsSaving())
  {
    RptF("DIAG-SERIALIZED BoundingCenter of %s: %.2f,%.2f", cc_cast(_name), _boundingCenter.X(),_boundingCenter.Z());
    extern bool GShrinkP3D;
    if (GShrinkP3D && (_boundingCenterGShrinkP3D!=_boundingCenter))
      ErrF(cc_cast(Format("Warning: Bounding center of %s has changed!",cc_cast(_name))));
  }
}

bool LODShape::PointsNeeded(float resolution)
{
  bool pointsNeeded = false;
  if( resolution>900 )
  {
    if( IsSpec(resolution,GEOMETRY_SPEC) ) pointsNeeded = true;
    else if( IsSpec(resolution,PATHS_SPEC) ) pointsNeeded = true;
    else if( IsSpec(resolution,VIEW_GEOM_SPEC) ) pointsNeeded = true;
    else if( IsSpec(resolution,FIRE_GEOM_SPEC) ) pointsNeeded = true;
    /* should internal view geometry be permanent ??? */
    else if( IsSpec(resolution,VIEW_PILOT_GEOM_SPEC) ) pointsNeeded = true;
    else if( IsSpec(resolution,VIEW_GUNNER_GEOM_SPEC) ) pointsNeeded = true;
    else if( IsSpec(resolution,VIEW_CARGO_GEOM_SPEC) ) pointsNeeded = true;
    /**/
  }
  return pointsNeeded;
}

bool LODShape::IsPermanent(float resolution)
{
  bool permanent = false;
  if( resolution>900 )
  {
    if( IsSpec(resolution,GEOMETRY_SPEC) ) permanent = true;
    else if( IsSpec(resolution,MEMORY_SPEC) ) permanent = true;
    else if( IsSpec(resolution,LANDCONTACT_SPEC) ) permanent = true;
    else if( IsSpec(resolution,ROADWAY_SPEC) ) permanent = true;
    else if( IsSpec(resolution,PATHS_SPEC) ) permanent = true;
    else if( IsSpec(resolution,HITPOINTS_SPEC) ) permanent = true;
    //else if( IsSpec(resolution,SHADOWVOLUME_SPEC) ) permanent = true;
    else if( IsSpec(resolution,VIEW_GEOM_SPEC) ) permanent = true;
    else if( IsSpec(resolution,FIRE_GEOM_SPEC) ) permanent = true;
    /* should internal view geometry be permanent ??? */
    else if( IsSpec(resolution,VIEW_PILOT_GEOM_SPEC) ) permanent = true;
    else if( IsSpec(resolution,VIEW_GUNNER_GEOM_SPEC) ) permanent = true;
    else if( IsSpec(resolution,VIEW_CARGO_GEOM_SPEC) ) permanent = true;
    // subparts lod will be permanent, it is not essentially needed and can be changed in the future into asynchronous lod.... 
    else if( IsSpec(resolution,SUB_PARTS_SPEC) ) permanent = true; 
    else if( IsSpec(resolution,WRECK_SPEC) ) permanent = true; 
  }
  return permanent;
}
bool Shape::OptimizePointToVertex(bool pointsNeeded)
{
  if (_pointToVertex.Size()==0) return true;
  if (pointsNeeded) // check for identity
  {
    for (int i=0; i<_pointToVertex.Size(); i++)
    {
      if (_pointToVertex[i]!=i) return false;
    }
  }
  _pointToVertex.Clear();
  _vertexToPoint.Clear();
  return true;
}

#include <Es/Algorithms/qsort.hpp>

/// p3d file level order definition

static int CompareLODOrder(const int *l1, const int *l2, const LODShape *shape)
{
  int lod1 = *l1;
  int lod2 = *l2;
  float resol1 = shape->Resolution(lod1);
  float resol2 = shape->Resolution(lod2);
  bool perm1 = shape->IsPermanent(resol1);
  bool perm2 = shape->IsPermanent(resol2);
  if (perm1!=perm2)
  {
    // permanent levels first
    return (int)perm2-(int)perm1;
  }
  // coarse levels first
  if (resol1>resol2) return -1;
  if (resol1<resol2) return +1;
  return 0;
}

static int CompareOffsetOrder(const int *l1, const int *l2, const int *offsets)
{
  int lod1 = *l1;
  int lod2 = *l2;
  return offsets[lod1]-offsets[lod2];
}

DEF_RSB(hybrid);

/*!
\patch_internal 1.53 Date 5/3/2002 by Ondra
- Optimized: Properties class and dammage are not stored lowercase.
Exact compare of RStringB or strcmp is therefore possible instead of strcmpi.
*/


void LODShape::SerializeBin(SerializeBinStream &f)
{
#ifdef _MSC_VER
  if (!f.Version('LODO'))
#else
  if (!f.Version(StrToInt("ODOL")))
#endif
  {
    f.SetError(f.EBadFileType);
    return;
  }
  // version 44 - LZW compression was replaced by LZO compression
  // version 45 - normals, ST and UV coordinates are stored discretized
  // version 46 - fix: normals, ST was not stored correctly in 45
  // version 47 - fix: ST computation was wrong in v. 46
  // version 48 - added shadow offset
  // version 49 - only make P3D unreadable in old ARMA2
  // version 50 - clip moved into discardable geometry
  // version 51 - nVertices and face area saved in the discardable header
  // version 52 - separate minmax for visual LODs
  const int actVersion = 52;
  const int wantedVersion = 31; // older version can be accepted, but is reported to the log file
  const int minVersion = 28;
  if (!f.Version(minVersion, actVersion))
  {
#if _VBS3
    if ((f.GetVersion() >= 10042) && (f.GetVersion() <= 10043))
    {
      LogF("Warning: Model %s is in VBS2 format.", Name());
    }
    else
#endif
    {
      LogF("Error: Model %s is in unsupported format %d (current format is %d, minimum supported is %d)", Name(), f.GetVersion(), actVersion, minVersion);
      f.SetError(f.EBadVersion);
      return;
    }
  }

  #if _ENABLE_REPORT
    if (f.GetVersion()<wantedVersion)
    {
      LogF("%s: Old model format %d",(const char *)GetName(),f.GetVersion());
    }
  #endif

  // starting from version 44, LZN compression was replaced by LZO compression
  if (f.GetVersion() >= 44) f.UseLZOCompression();

  // save/load all shapes
  if (f.IsLoading())
  {
    _nLods = f.LoadInt();

    _streaming = true;

    
    if (_streaming)
    {
      f.LoadLittleEndian(_resolutions,NLevels(),sizeof(*_resolutions));
      SerializeBinHeaders(f);
      ScanResolutions();
      
      if (f.GetVersion()>=30)
      {
        // check if animations are saved
        bool anims;
        f.Transfer(anims);
        if (anims)
        {
          _animations = new AnimationHolder;
          _animations->SerializeBin(f);
          for (int level=0; level<NLevels(); level++)
          {
            _animations->SerializeBin(f,level);
          }
        }
      }
      
      AUTO_STATIC_ARRAY(int,levelOrder,64);
      AUTO_STATIC_ARRAY(int,levelBegOffset,64);
      AUTO_STATIC_ARRAY(int,levelEndOffset,64);
      AUTO_STATIC_ARRAY(bool,permanent,64);
      levelOrder.Resize(NLevels());
      for (int i=0; i<NLevels(); i++ ) levelOrder[i] = i;
      levelBegOffset.Resize(NLevels());
      levelEndOffset.Resize(NLevels());
      permanent.Resize(NLevels());
      f.LoadLittleEndian(levelBegOffset.Data(),levelBegOffset.Size(),sizeof(int));
      f.LoadLittleEndian(levelEndOffset.Data(),levelEndOffset.Size(),sizeof(int));
      f.Load(permanent.Data(),permanent.Size()*sizeof(bool));
#if LOG_SHAPE_LOADING
      // load all now to create a better log
      for (int i=0; i<NLevels(); i++) permanent[i] = true;
#endif
      // sort order based on level offset
      QSort(levelOrder.Data(),levelOrder.Size(),levelBegOffset.Data(),CompareOffsetOrder);
      
      for (int level=0; level<NLevels(); level++)
      {
        if (permanent[level]) continue;

        // we need to get handle from the file
        QFBankHandle handle = QFBankQueryFunctions::GetHandle(
          _name,levelBegOffset[level],levelEndOffset[level]-levelBegOffset[level]
        );

        _lods[level] = new ShapeRefLoadable(handle,this,level,f,f.GetVersion()
#ifdef _M_PPC
          ,f.GetEndianState()
#endif
          , f.IsUsingLZOCompression()
        );
        //if (managed) Shapes.ShapeUnloaded(managed);
        
      }
      for (int i=0; i<NLevels(); i++)
      {
        int level = levelOrder[i];
        // TODO: do not load geometry by default
        if (permanent[level])
        {
          f.SeekG(levelBegOffset[level]);
          int begGeom=0,endGeom=0;
          int begFaces=0,endFaces=0;
          Shape *nlod = new Shape;
          
          #if NAMED_SEL_STATS
            //NamedSelCreateContext = LODShape::LevelName(_resolutions[level]);
            NamedSelCreateContext = GetName()+"-"+LODShape::LevelName(_resolutions[level]);
          #endif

#if LOG_SHAPE_LOADING
          LogF("Loading shape %s, level %d - offset %d", cc_cast(Name()), level, f.TellG());
#endif
          nlod->SerializeBin(f,true,begGeom,endGeom,begFaces,endFaces,this);
          #if NAMED_SEL_STATS
            NamedSelCreateContext = RString();
          #endif
          DoAssert(levelEndOffset[level]==f.TellG());
          
          if (f.GetVersion()<51)
            nlod->RecalculateFaceArea(); // for streaming it will be recalculated later, but for permanent ones we need to do it now
          _lods[level].SetPermanentRef(nlod);
          // no load handle needed for permanent lods
          
          float resolution = _resolutions[level];
          // check if we need pointToVertex conversion
          bool pointsNeeded = PointsNeeded(resolution);
          
          bool pointsAway = _lods[level].GetRef()->OptimizePointToVertex(pointsNeeded);
          if (!pointsAway)
          {
            LogF(
              "Non-trivial point to vertex conversion in %s:%s",
              (const char *)GetName(),LevelName(resolution)
            );
          }
        }

        #if _ENABLE_REPORT
          {
            Shape *lod = GET_LOD(_lods[level]);
            if (lod && lod->_loadWarning)
            {
              LogF("%s (%s): warnings",Name(),LevelName(_resolutions[level]));
            }
          }
        #endif
      }
      
      // we will read from the file no more, 
      // skip after last lod level
      f.SeekG(levelEndOffset[levelOrder[NLevels()-1]]);
      
    }
    else
    {
      for (int l=0; l<NLevels(); l++)
      {
        Shape *nlod = new Shape;
        int begGeom,endGeom;
        int begFaces,endFaces;
        nlod->SerializeBin(f,true,begGeom,endGeom,begFaces,endFaces,this);
        //nlod->SetLevel(l);

        _lods[l] = nlod;
      }
      f.LoadLittleEndian(_resolutions,NLevels(),sizeof(*_resolutions));
    }

    if (f.GetVersion()<52 && f.IsLoading())
    {
      CalculateMinMaxVisual();
    }

    RescanLodsArea();
  }
  else
  {
    f.SaveInt(NLevels());
    f.TransferBinary(_resolutions,NLevels()*sizeof(*_resolutions));
    SerializeBinHeaders(f);
    
    bool anims = _animations.NotNull();
    f.Transfer(anims);
    if (_animations)
    {
      _animations->SerializeBin(f);
      for (int level=0; level<NLevels(); level++)
      {
        _animations->SerializeBin(f,level);
      }
    }
      
    // save permanent resolutions first, graphical later, low res first
    AUTO_STATIC_ARRAY(int,levelOrder,64);
    levelOrder.Resize(NLevels());
    for (int i=0; i<NLevels(); i++) levelOrder[i] = i;
    // sort to get a save order
    QSort(levelOrder.Data(),levelOrder.Size(),this,CompareLODOrder);
    // create a fixup space for level offsets
    AUTO_STATIC_ARRAY(int,begFixups,64);
    AUTO_STATIC_ARRAY(int,endFixups,64);
    AUTO_STATIC_ARRAY(int,begOffset,64);
    AUTO_STATIC_ARRAY(int,endOffset,64);
    begFixups.Resize(NLevels());
    endFixups.Resize(NLevels());
    begOffset.Resize(NLevels());
    endOffset.Resize(NLevels());
    AUTO_STATIC_ARRAY(bool,permanent,64);
    permanent.Resize(NLevels());
    for (int i=0; i<NLevels(); i++)
    {
      begFixups[i] = f.CreateFixUp();
    }
    for (int i=0; i<NLevels(); i++)
    {
      endFixups[i] = f.CreateFixUp();
    }
    for (int i=0; i<NLevels(); i++)
    {
      permanent[i] = (
        IsPermanent(_resolutions[i]) ||
        i==_nGraphical-1 && (_nGraphical>1 || _lods[i]->NFaces()<100)
      );
    }
    f.Save(permanent.Data(),permanent.Size()*sizeof(bool));
    // for each non-permanent lod basic info is saved
    for (int i=0; i<NLevels(); i++)
    {
      if (permanent[i]) continue;
      // corresponds to ShapeRefLoadable - search for LOAD_REF
      Shape *level = GET_LOD(_lods[i]);
      f.SaveInt(level->NFaces());
      f.SaveInt(level->GetColor());
      f.SaveInt(level->Special());
      f.SaveInt(level->GetOrHints());
      f.SaveChar(level->GetSubSkeletonSize()>0);
      f.SaveInt(level->NVertex());
      float faceArea = level->TotalFaceArea();
      f.Save(&faceArea,sizeof(faceArea));
    }
    for (int i=0; i<NLevels(); i++)
    {
      int level = levelOrder[i];
      int begGeom,endGeom;
      int begFaces,endFaces;
      begOffset[level] = f.TellP();
      _lods[level]->SerializeBin(f,true,begGeom,endGeom,begFaces,endFaces,this);
      endOffset[level] = f.TellP();
    }
    for (int i=0; i<NLevels(); i++)
    {
      f.FixUp(begFixups[i],begOffset[i]);
      f.FixUp(endFixups[i],endOffset[i]);
    }
  }

  // check if shadow resolution is reasonable
  // if not, fix it
  DoAssert (_minShadow>=0);
  
  const int wantedShadowFaces = 1000;
  const int maxShadowFaces = 2000;
  if (_minShadow>=NLevels())
  {
    // OK - shadow disabled
  }
  else if( _resolutions[_minShadow]>=900)
  {
    // shadow points to some special LOD - this means there is no shadow
    _minShadow = NLevels();
  }
  else if (_lods[_minShadow].NFaces()>=wantedShadowFaces)
  {
    // shadow is ok, but it is too detailed - report problem
//    LogF(
//      "Too complex shadow in %s (%d)",(const char *)GetName(),
//      _lods[_minShadow].NFaces()
//    );
    // fix suitable lod
    for(int i=_minShadow; i<_nLods; i++ )
    {
      if (_resolutions[i]>900)
      {
        if (i>_minShadow)
        {
          _minShadow = i-1;
        }
        break; // only normal LODs can be used for shadowing
      }
      // disable shadows of too complex LODs
      if (_lods[i].NFaces()<wantedShadowFaces)
      {
        _minShadow = i;
        break;
      }
    }
    if (_minShadow<_nLods)
    {
      int nFaces = _lods[_minShadow].NFaces();
      if (nFaces>=maxShadowFaces)
      {
        RptF(
          "Too detailed shadow lod in %s (%d:%f : %d) - shadows disabled",
          (const char *)GetName(),
          _minShadow,Resolution(_minShadow),nFaces
        );
        _minShadow = _nLods;
      }
      else
      {
//        LogF("  Shadow used %d",_lods[_minShadow].NFaces());
      }
    }
  }

  if (f.IsLoading())
  {
    ScanProjShadow(); // make sure projected shadow flags are set
    OptimizeShapes(); // remove lods not needed
    // note: we can no longer access model config outside of binarize
    //CheckForcedProperties();
    InitConvexComponents(false);
    #if _ENABLE_REPORT
      // verify no land-surface flags are used
      static RStringB SlopeProp = "slope";
      static RStringB PlacementProp = "placement";
      if (GeometryLevel() && GeometryLevel()->PropertyValue(PlacementProp)==SlopeProp)
      {
        if (GetOrHints()&ClipLandMask)
        {
          RptF("%s: bogus land flags %x",(const char *)GetName(),GetOrHints());
        }
      }
      MapType map = GetMapType();
      if (map==MapTree || map==MapSmallTree || map==MapBush || map==MapRock)
      {
        if (GetOrHints()&ClipLandMask)
        {
          RptF("%s: wrong land flags %x",(const char *)GetName(),GetOrHints());
        }
      }
    #endif
    // recalculate view density

    float alpha = _color.A8()*(1.0/255);
    //float transparency = 1-alpha*2;
    float transparency = 1-alpha*1.5;
    
    if (transparency>=0.99) _viewDensity = 0;
    if (transparency>0.01) _viewDensity = log(transparency)*4;
    else _viewDensity = -100;

    // Estimate the _preferShadowVolume in case of old file
    if (f.GetVersion() < 37)
    {
      if (_geometry>=0 && PropertyValue("shadow") == RSB(hybrid))
      {
        _preferShadowVolume = false;
      }
    }
    
    // we cannot do this in SerializeBinHeaders, as properties are not loaded there yet
    if (f.GetVersion()<48)
    {
      ReadShadowOffsetFromProperty(_shadowOffset);
    }

    AutodetectShadowOffsetIfNeeded();

    // Get the last geometry level and create the shadow geometry
    _shadowVolume = FindSpecLevelInterval(_shadowVolumeCount, SHADOWVOLUME_SPEC, SHADOWVOLUME_MAX);

    // Get the shadow buffer references
    _shadowBuffer = FindSpecLevelInterval(_shadowBufferCount, SHADOWBUFFER_SPEC, SHADOWBUFFER_MAX);

    // make sure all LODs pass post-load processing
    for (int i=0; i<NLevels(); i++)
    {
      if (GetLevelLocked(i))
      {
        ShapeLoaded(i);
      }
    }

  }

  // check for suspicious face counts
  // if better lod has less faces than a worse one, it is strange
  #if _ENABLE_REPORT
    for (int i=1; i<_nGraphical; i++)
    {
      int thisComplex = Complexity(i);
      float thisResol = _resolutions[i];
      float minResol = toIntFloor(thisResol/100)*100;
      for (int j=i-1; j>=0; j--)
      {
        if (_resolutions[j]<minResol) break;
        int prevComplex = Complexity(j);
        if (prevComplex<=thisComplex && (prevComplex>0 || thisComplex>0))
        {
          // assume each 0...100 constitutes a separate interval
          RString thisName = LevelName(thisResol);
          RString prevName = LevelName(_resolutions[j]);
          RptF(
            "Warning: '%s': LODs not ordered by face count %d (%s): %d faces < %d (%s) %d faces",
            cc_cast(GetName()),
            j,cc_cast(prevName),prevComplex,
            i,cc_cast(thisName),thisComplex
          );
        }
      }
      
    }
  #endif
}

bool LODShape::Preload(FileRequestPriority prior, const char *name)
{
  QIFStreamB in;
  in.AutoOpen(name);
  // currently whole shape data are needed when loading object - this should be changed
  if (in.fail())
  {
    RptF("LODShape::Preload: shape '%s' not found",name);
    return true;
  }
  bool shapeData = in.PreRead(prior);
  if (!shapeData) return false;
  // whole shape and loader header are ready
  return true;
}

bool LODShape::CheckLoadHandler(const LODShapeLoadHandler *handler) const
{
  for (int i=0; i<_loadHandler.Size(); i++)
  {
    if (_loadHandler[i]==handler)
    {
      return true;
    }
  }
  return false;
}

#if _ENABLE_CHEATS
static char InterestedIn[256]="ca\\ui\\face_preview.p3d";
#endif

void LODShape::AddLoadHandler(LODShapeLoadHandler *handler)
{
  REPORTSTACKITEM(GetName());

  #if 0 // _ENABLE_CHEATS
  if (!strcmp(GetName(),InterestedIn))
  {
    LogF("## Shape %s AddLoadHandler",cc_cast(GetName()));
  }
  #endif
  // if the handler is already here, no need to add it and no need to run it
  for (int i=0; i<_loadHandler.Size(); i++)
  {
    if (_loadHandler[i]==handler)
    {
      return;
    }
  }

  // if someone is adding a handler, some reference must exist
  Assert(RefCounter()>0);
  handler->LODShapeLoaded(this);
  for (int i=0; i<NLevels(); i++)
  {
    if (!LevelRef(i).IsLoaded()) continue;
    handler->ShapeLoaded(this, i);
  }
  _loadHandler.Add(handler);
  handler->HandlerRegistered();
}
void LODShape::RemoveLoadHandler(LODShapeLoadHandler *handler)
{
  #if 0 //_ENABLE_CHEATS
  if (!strcmp(GetName(),InterestedIn))
  {
    LogF("## Shape %s RemoveLoadHandler",cc_cast(GetName()));
  }
  #endif
  if (!_loadHandler.Delete(handler))
  {
    Fail("Handler not present");
    return;
  }
  // pretend the shape is loaded for a while
  TempAddRef();
  handler->HandlerUnregistered();
  for (int i=0; i<NLevels(); i++)
  {
    if (!LevelRef(i).IsLoaded()) continue;
    handler->ShapeUnloaded(this, i);
  }
  handler->LODShapeUnloaded(this);
  TempRelease();
}

void LODShape::ShapeLoaded(int level)
{
  #if 0 //_ENABLE_CHEATS
  if (!strcmp(GetName(),InterestedIn))
  {
    LogF("## Shape %s ShapeLoaded %d",cc_cast(GetName()),level);
  }
  #endif
  // Create the subskeleton
  // TODO: check if modification of shape is legal
  ShapeUsed lock = Level(level);
  Assert(lock.NotNull());
  Shape *shape = lock.InitShape();
  if (_skeletonSource.NotNull() && (_skeletonSource->Structure().Size() > 0))
  {
    if (shape->_skeletonToSubSkeleton.Size()==0 && shape->_subSkeletonToSkeleton.Size()==0)
    {
      ErrF(
        "Obsolete skeletonSource %s for %s",
        cc_cast(_skeletonSource->GetName()),cc_cast(GetName())
      );
    }
  }

  //DoAssert(_lods[level].GetShapeRef()->GetManagedInterface());
  if (IsSpecShadowVolume(Resolution(level)))
  {
    Ref<TexMaterial> material = GlobPreloadMaterial(ShadowMaterial);
    shape->SetUniformMaterial(material, NULL);
    shape->VertexDeclarationNeedsUpdate();
    shape->DisablePushBuffer();
  }
  
  shape->UpdateShapePropertiesNeeded();
  if (!_treeCrown && (shape->GetShapePropertiesNeeded()&ESPTreeCrown))
  {
    CalculateTreeCrown();
  }
    
  for (int i=0; i<_loadHandler.Size(); i++)
  {
    _loadHandler[i]->ShapeLoaded(this, level);
  }
}
void LODShape::ShapeUnloaded(int level)
{
  #if 0 //_ENABLE_CHEATS
  if (!strcmp(GetName(),InterestedIn))
  {
    LogF("## Shape %s ShapeUnloaded %d",cc_cast(GetName()),level);
  }
  #endif
  DoAssert(_lods[level].GetShapeRef()->GetManagedInterface());
  for (int i=0; i<_loadHandler.Size(); i++)
  {
    _loadHandler[i]->ShapeUnloaded(this, level);
  }
}


/*!
\patch 1.08 Date 7/20/2001 by Ondra.
- Fixed: Crash when p3d file was corrupted. Error message is shown instead.
*/

bool LODShape::LoadOptimized(QIStream &f)
{
  SerializeBinStream str(&f);
  SerializeBin(str);
  if (str.GetError()==str.EBadFileType) return false;
  // some version error
  switch(str.GetError())
  {
    case SerializeBinStream::EOK:
      break;
    case SerializeBinStream::EBadVersion:
      ErrorMessage("Bad version %d in p3d file '%s'",str.GetVersion(),(const char *)GetName());
      break;
    default:
      ErrorMessage("Bad file format in p3d file '%s'.",(const char *)GetName());
      break;
  }
  return true;
}
void LODShape::SaveOptimized(QOStream &f)
{
  SerializeBinStream str(&f);
  SerializeBin(str);
}

bool LODShape::LoadOptimized(const char *name)
{
  //_name = name;
  QIFStreamB f;
  f.AutoOpen(name);
  return LoadOptimized(f);
}


void LODShape::SaveOptimized(const char *name)
{
  QOFStream f;
  f.open(name);
  SaveOptimized(f);
  f.close();
}

