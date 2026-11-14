// Poseidon - shadow casting
// (C) 1997, SUMA
#include "wpch.hpp"

#include "object.hpp"
#include "scene.hpp"
#include "Shape/poly.hpp"
#include "tlVertex.hpp"
#include "global.hpp"
#include "engine.hpp"
#include "landscape.hpp"
#include "world.hpp"
#include "lights.hpp"
#include "camera.hpp"
#include "timeManager.hpp"
#include <El/Common/perfProf.hpp>
#include <El/Common/perfLog.hpp>

//#define MAX_CACHE_SHADOWS 512

DEFINE_FAST_ALLOCATOR(RememberSplit)

RememberSplit::RememberSplit()
:_object(NULL)
{
}

RememberSplit::~RememberSplit()
{
}


size_t RememberSplit::GetMemoryControlled() const
{
  size_t used = _shadow ? _shadow->GetMemoryControlledByShape(true) : 0;
  return used+sizeof(*this);
}

void RememberSplit::InitSplit(Object *object, int level, int special)
{
  TimeManagerScope scope(TCPrepareShadow);
  PROFILE_SCOPE(shdIn);
  
  DoAssert(CheckMainThread());
  
  // such shapes also preserve original y offset
  _object = object;

  Assert(IsEqualObject(object));

  //_coordinates=*object;
  _objectPos = object->RenderVisualState().Transform();
  _level=level;

    PROFILE_SCOPE(shdSO);
    // match terrain tessellation - used for roads
    // copy source data
    LODShape *lshape = object->GetShape();
  ShapeUsedGeometryLock<> source(lshape,level);
    // we need to keep ST if it was in the original model
    // we are not interested in any selections
    _shadow = new Shape(*source.GetShape(), false, false, true);
    Assert(_shadow->CheckIntegrity());

  if (_shadow)
  {
    if (special&OnSurface)
    {
      PROFILE_SCOPE(shdSS);
      _shadow->SurfaceSplit(GLandscape,object->RenderVisualState().Transform(),GEngine->ZShadowEpsilon(),object->RenderVisualState().Scale());
      // it is already fitted - prevent future surface fitting
      _shadow->CalculateMinMax();
      _shadow->AndSpecial(~OnSurface);
      _shadow->OrSpecial(IsOnSurface);
    }
    // make sure buffer is regenerated
    PROFILE_SCOPE(shdVB);
    _shadow->ReleaseVBuffer(true);
    // optimize it for HW rendering
    // sections are created directly during SurfaceSplit
    //_shadow->FindSections();
    if (_shadow->NVertex()>256)
    {
      _shadow->ConvertToVBuffer(VBBigDiscardable, object->GetShape(), level);
    }
    else
    {
      _shadow->ConvertToVBuffer(VBSmallDiscardable, object->GetShape(), level);
    }

    // table should be exact - for best memory usage
    _shadow->Compact();
    // unload geometry, as it is now no longer needed
    //_shadow->UnloadGeometry();
  }
  // note: some more memory may be used in vertex buffer
  // however with optimal handling there should be always one copy of static data
  // - vbuffer or Shape
}

DEFINE_FAST_ALLOCATOR(ShadowIndex)

void Object::RemoveAllShadows()
{
#if _BINARIZE_REDUCEOBJECT
  Fail("Waring: Object functionality reduced");
#else
  _shadow.Free();
#endif
}

RememberSplit *Object::GetShadowReady(int level) const
{
#if _BINARIZE_REDUCEOBJECT
  Fail("Waring: Object functionality reduced");
#else
  if (!_shadow) return NULL;
  int bestDiff = INT_MAX;
  int bestI = -1;
  for (int i=0; i<MAX_LOD_LEVELS; i++)
  {
    RememberSplit *index = _shadow->_lods[level];
    if (!index || !index->IsLoaded())
    {
      continue;
    }
    int diff = abs(i-level);
    if (bestDiff>diff) bestDiff = diff, bestI =i;
  }
  if (bestI>=0)
  {
    return _shadow->_lods[bestI];
  }
#endif
  return NULL;
}

RememberSplit *Object::GetShadow(int level) const
{
#if _BINARIZE_REDUCEOBJECT
  Fail("Waring: Object functionality reduced");
  return NULL;
#else
  if (!_shadow) return NULL;
  return _shadow->_lods[level];
#endif
}

void Object::RemoveShadow(int level)
{
#if _BINARIZE_REDUCEOBJECT
  Fail("Waring: Object functionality reduced");
#else
  #if !_RELEASE
  if (!_shadow->_lods[level])
  {
    Fail("Removed non-existing shadow");
  }
  #endif
  _shadow->_lods[level] = NULL;
  if (--_shadow->_nShadows<=0)
  {
    // last shadow removed from the index
    _shadow.Free();
  }
#endif
}

void Object::SetShadow(int level, RememberSplit *shadow)
{
#if _BINARIZE_REDUCEOBJECT
  Fail("Waring: Object functionality reduced");
#else
  if (!_shadow)
  {
    _shadow = new ShadowIndex;
    _shadow->_nShadows = 0;
  }
  if (!_shadow->_lods[level])
  {
    _shadow->_nShadows++;
  }
  _shadow->_lods[level] = shadow;
#endif
}


ShadowCache::ShadowCache()
{
  RegisterFreeOnDemandMemory(this);
}

ShadowCache::~ShadowCache()
{
  DoAssert(_data.MemoryControlled() == 0);
}


Ref<Shape> ShadowCache::Split(Object *object, Vector3Par lightDir, int level, int special)
{
  SCOPE_GRF("shadow",0);
  
  AssertMainThread();
  Assert(CheckIntegrity(object));

  RememberSplit *si = object->GetShadow(level);

  // if we split, we need to do it always - we can limit only shadow casting 
  bool canGenerate = true; // TODO: remove TCPrepareShadow
  if (si)
  {
    if (!canGenerate)
    {
      // no time left for shadow calculation - if we have a shadow ready, use it, if not, skip it
      if (!si->IsLoaded())
      {
        si = object->GetShadowReady(level);
      }
      if (!si) return NULL;
      _data.Refresh(si);
    }
    else
    {
      Assert(CheckIntegrity(si));

      const float maxPosDiff=0.02f;
      if (!si->IsLoaded() || object->RenderVisualState().Transform().Distance2(si->ObjectPos())>Square(maxPosDiff))
      {
        // create temporary Ref to make sure entry is not released prematurely
        Ref<RememberSplit> temp=si;
        // both static and dynamic object shadows are cached
        _data.Delete(si);
        si->InitSplit(object,level,special);
        _data.Add(si);
      }
      else
      {
        // reorder shadow cache
        _data.Refresh(si);
      }
      //_data.Insert(si);
      Assert(object->GetShadow(level)==si);
      Assert(CheckIntegrity(si));
      Assert(CheckIntegrity(object));
    }

    if (si->_shadow)
    {
      Assert(si->_shadow->GetVertexBuffer());
      if (si->_shadow->NVertex()>256)
      {
        si->_shadow->ConvertToVBuffer(VBBigDiscardable, object->GetShape(), level);
      }
      else
      {
        si->_shadow->ConvertToVBuffer(VBSmallDiscardable, object->GetShape(), level);
      }
    }
    
    return si->_shadow;
  }
  else if (canGenerate)
  {
    // a new shadow
    // _data is sorted by time when it was used
    Ref<RememberSplit> entry=new RememberSplit;
    entry->InitSplit(object,level,special);
    DoAssert(entry->IsEqualObject(object));
    // create a new entry
    _data.Add(entry);
    object->SetShadow(level,entry);
    Assert(CheckIntegrity(entry));
    Assert(CheckIntegrity(object));

    return entry->_shadow;
  }
  else
  {
    // try to reuse a shadow
    si = object->GetShadowReady(level);
    if (si && si->_shadow)
    {
      if (si->_shadow->NVertex()>256)
      {
        si->_shadow->ConvertToVBuffer(VBBigDiscardable, object->GetShape(), level);
      }
      else
      {
        si->_shadow->ConvertToVBuffer(VBSmallDiscardable, object->GetShape(), level);
      }
      return si->_shadow;
    }
    return NULL;
  }
}

bool RememberSplit::IsLoaded() const
{
  if (!_shadow) return true;
  return _shadow->GetVertexBuffer() || _shadow->IsGeometryLoaded();
}

void RememberSplit::UnloadGeometry()
{
  if (_shadow && _shadow->GetVertexBuffer())
  {
    _shadow->UnloadGeometry();
  }
}

void ShadowCache::CleanUp()
{
  // old entries are listed at the end
  RememberSplit *entry;
  while( (entry=_data.First())!=NULL )
  {
    // TODO: if the light direction is very different, we may safely remove the shadow
    // as it cannot be used anyway
    if( entry->_object.NotNull() && entry->IsLoaded() ) break;
    
    entry->_object.Lock();
    if (entry->_object)
    {
      entry->_object->RemoveShadow(entry->_level);
    }
    entry->_object.Unlock();
    _data.Delete(entry);
  }
}

const float ShadowMemSize = 10*1024;

bool ShadowCache::CheckIntegrity(Object *obj) const
{
  bool ret = true;
  if (!obj->CheckIntegrity())
  {
    ret = false;
  }
  ShadowIndex *index = obj->GetShadowIndex();
  if (!index) return ret;
  LODShape *lShape = obj->GetShape();
  if (!lShape) return ret;
  for (int i=0; i<lShape->NLevels(); i++)
  {
    RememberSplit *s = obj->GetShadow(i);
    if (!s) continue;

    if (!s->IsInList())
    {
      RptF("shadow not in the list for %s:%d",(const char *)obj->GetDebugName(),i);
      ret = false;
    }
    // check if back pointer is correctly linked
    if (!s->IsEqualObject(obj))
    {
      RptF(
        "s->_object corrupted for %s:%d shadow (%s:%d)",
        (const char *)obj->GetDebugName(),i,
        (const char *)s->_object.GetDebugName(),
        s->_level
      );
      ret = false;
    }
    else if (obj->GetShadow(s->_level)!=s)
    {
      RptF("obj->GetShadow backlink corrupted for %s:%d",(const char *)obj->GetDebugName(),i);
      ret = false;
    }
    else if (s->_level!=i)
    {
      RptF("Level corrupted for %s:%d shadow: %d",(const char *)obj->GetDebugName(),i,s->_level);
      ret = false;
    }
  }
  return ret;
}

bool ShadowCache::CheckIntegrity(RememberSplit *sc) const
{
  bool ret = true;
  #if 0
    sc->_object.Lock();
    Object *obj = sc->_object;
    if (obj && obj->GetShadow(sc->_level)!=sc)
    {
      RptF("obj->GetShadow corrupted for %s:%d",(const char *)obj->GetDebugName(),sc->_level);
      ret = false;
    }
    sc->_object.Unlock();
  #endif
  return ret;
}

bool ShadowCache::CheckIntegrity(bool detailed) const
{
  bool ret = true;
  for( RememberSplit *s=_data.First(); s; s=_data.Next(s) )
  {
    // check if everything is valid
    if (!CheckIntegrity(s))
    {
      ret = false;
    }
  }
  // scan all landscape objects and check if corresponding entries do exist
  // scam only visible area around the player
  #if _ENABLE_REPORT
  if (detailed && GScene && GScene->GetCamera())
  {
    int maxRange = toIntCeil(Glob.config.objectsZ*InvLandGrid);
    int x0 = toInt(GScene->GetCamera()->Position().X()*InvLandGrid);
    int z0 = toInt(GScene->GetCamera()->Position().Z()*InvLandGrid);
    int xmin = x0-maxRange, xmax = x0+maxRange;
    int zmin = z0-maxRange, zmax = z0+maxRange;
    saturateMax(xmin,0); saturateMin(xmax,LandRange-1);
    saturateMax(zmin,0); saturateMin(zmax,LandRange-1);
    for( int x=xmin; x<=xmax; x++ ) for( int z=zmin; z<=zmax; z++ )
    {
      const ObjectList &list=GLandscape->GetObjects(z,x);
      for( int o=0; o<list.Size(); o++ )
      {
        Object *obj=list[o];
        if (!CheckIntegrity(obj))
        {
          ret = false;
        }
      }
    }
  }
  #endif
  return ret;
}

void ShadowCache::MemoryControlledFrame()
{
  _data.Frame();
}

size_t ShadowCache::FreeOneItem()
{
  for( RememberSplit *entry=_data.First(); entry; entry=_data.Next(entry) )
  {
    Assert(CheckIntegrity(entry));
    // only Ref to entry should normally be from the cache
    if (entry->RefCounter()>1)
    {
      LogF("Shadow locked");
      continue;
    }

    entry->_object.Lock();
    Object *obj = entry->_object;
    if (obj)
    {
      obj->RemoveShadow(entry->_level);
    }
    entry->_object.Unlock();
    size_t released = _data.Delete(entry);
    return released;
  }
  return 0;
}

RString ShadowCache::GetDebugName() const
{
  return "Shadows";
}

float ShadowCache::Priority() const
{
  return 0.2f;
}

size_t ShadowCache::MemoryControlled() const
{
  #if _ENABLE_REPORT
  Assert( _data.CheckIntegrity() );
  #endif
  return _data.MemoryControlled();
}
size_t ShadowCache::MemoryControlledRecent() const
{
  return _data.MemoryControlledRecent();
}


void ShadowCache::ShadowChanged(Object *obj)
{
  // remove all shadows cached for this object
  ShadowIndex *index = obj->GetShadowIndex();
  if (!index) return;
  for (int i=0; i<MAX_LOD_LEVELS; i++)
  {
    RememberSplit *entry = index->_lods[i];
    if (!entry) continue;
    Assert(CheckIntegrity(entry));
    Assert(entry->_object.IsEqual(obj));
    _data.Delete(entry);
  }
  obj->RemoveAllShadows();

  Assert(CheckIntegrity(obj));
}

void ShadowCache::Clear()
{
  Assert(CheckIntegrity(true));
  for( RememberSplit *sc=_data.First(); sc; sc=_data.Next(sc) )
  {
    Assert(CheckIntegrity(sc));

    Assert(sc->_object.IsReady());
    sc->_object.Lock();
    Object *obj = sc->_object;
    if (obj)
    {
      obj->RemoveShadow(sc->_level);
    }
    Shape *sh = sc->_shadow;
    if (sh) sh->ReleaseVBuffer();
    sc->_object.Unlock();
  }

  _data.Clear();
  // clear shadow indices for all objects in landscape
  #if !_RELEASE
  for( int x=0; x<LandRange; x++ ) for( int z=0; z<LandRange; z++ )
  {
    const ObjectList &list=GLandscape->GetObjects(z,x);
    for( int o=0; o<list.Size(); o++ )
    {
      Object *obj=list[o];
      if(obj->GetShadowIndex())
      {
        ErrF("Late ShadowIndex release - %s",(const char *)obj->GetDebugName());
        obj->RemoveAllShadows();
      }
    }
  }
  #endif
  Assert(CheckIntegrity(true));
}

#if _VBS3_CRATERS_DEFORM_TERRAIN

void ShadowCache::Clear2DVicinity(Vector3Par xzPosition, float xzRadius)
{
  Assert(CheckIntegrity(true));

  // Go through all cache items, find the ones in specified vicinity and remove them
  RememberSplit *sc=_data.First();
  while (sc)
  {
    Assert(CheckIntegrity(sc));
    Assert(sc->_object.IsReady());
    sc->_object.Lock();
    Object *obj = sc->_object;
    bool removeItem = false;
    if (obj)
    {
      if (xzPosition.DistanceXZ2(obj->RenderVisualState().Position()) < Square(xzRadius + obj->GetRadius()))
      {
        removeItem = true;
        obj->RemoveShadow(sc->_level);
        Shape *sh = sc->_shadow;
        if (sh) sh->ReleaseVBuffer();
        sc->_object.Unlock();
      }
    }

    if (removeItem)
    {
      // Get pointer to next item
      RememberSplit *next = _data.Next(sc);

      // Delete the current item
      _data.Delete(sc);

      // Move to the next item
      sc = next;
    }
    else
    {
      sc = _data.Next(sc);
    }
  }
  Assert(CheckIntegrity(true));
}

#endif

Ref<Shape> Object::RecalcShadow(int level, const FrameBase &frame)
{
  // all polygons cast shadows
  // project all points to ground level
  Matrix4Val iTrans=frame.GetInvTransform();
  float y=GEngine->ZShadowEpsilon();

  Ref<LODShape> shadow = _shape->MakeShadow(level);
    Assert(!shadow || shadow->GetLevelLocked(level) && shadow->Level(level)->CheckIntegrity());

  //if (shadow.IsNull()) return NULL;
  if (!shadow) return NULL;
  // we know shadow levels are permanent
  Ref<Shape> shape = shadow->InitLevelLocked(level);
  // there is no need to lock shadow, as it cannot be unloaded
  if( shape && shape->NFaces()>=0 )
  {
    DoAssert(shape->_source.IsNull());
    DoAssert(shape->IsGeometryLoaded());
    //ShapeUsedGeometryLock<> lock(shape);
    Assert(shape->CheckIntegrity());
    ShapeUsedGeometryLock<> orig(_shape,level);

    Vector3Val lightDir=GScene->MainLight()->ShadowDirection();
    Vector3 modelLightDir=iTrans.Rotate(lightDir);
    if
    (
      (orig->GetAndHints()&ClipDecalMask)==ClipDecalVertical &&
      (orig->GetOrHints()&ClipDecalMask)==ClipDecalVertical
    )
    {
      // special case - shadow casted by vertical decal
      LogF("Vertical decal shadow obsolete");
      return NULL;
    }
    else
    {
      FaceArray dest;
        dest.ReserveFaces(1024,false);
        // reserve same size as source
        dest.ReserveRaw(shape->Faces().RawSize());

      for (int s=0; s<shape->NSections(); s++)
      {
        const ShapeSection &ss = shape->GetSection(s);
        const ShapeSection &origs = orig->GetSection(s);
        if( (ss.Special()&(NoShadow|IsHidden|IsHiddenProxy)) )
        {
          // section will not be included in the destination
          continue;
        }
        ShapeSectionInfo ds = ss;
        if( ds.Special()&(IsAlpha|IsTransparent) ) ds.SetTexture(origs.GetTexture());
        else ds.SetTexture(NULL);
        for( Offset f=ss.beg; f<ss.end; shape->NextFace(f) )
        {
          //const Poly &sFace=orig->Face(f);
          Poly &face=shape->Face(f);
          // make sure both sFace and face are valid face pointers
          if( face.N()<3 ) continue;
          // world coordinates of face vertices
          Assert( face.GetVertex(0)<orig->NVertex() );
          Assert( face.GetVertex(1)<orig->NVertex() );
          Assert( face.GetVertex(2)<orig->NVertex() );
          Vector3Val v0=orig->Pos(face.GetVertex(0));
          Vector3Val v1=orig->Pos(face.GetVertex(1));
          Vector3Val v2=orig->Pos(face.GetVertex(2));
          Vector3Val v10 = v1-v0;
          Vector3 normal=v10.CrossProduct(v2-v0);
          float check=modelLightDir*normal;
          if( check>=0 )
          {
            }
          else
          {
              // face should be included in shadow - add it to destination storage
              dest.Add(face);
            }
          }
        dest.CloseSection(ds);
      }
      bool ok = true;
      float maxShadowSize = Glob.config.GetMaxShadowSize();
      for( int i=0; i<shape->NVertex(); i++ )
      {
        // calculate world space position of shadow
        Vector3Val origPos=orig->Pos(i);
        Vector3 pPos(VFastTransform,frame.Transform(),origPos);
        Vector3 shadowPos;
        if (!GScene->ShadowPos(pPos,shadowPos,GScene->MainLight(),maxShadowSize))
        {
          ok = false;
        }
        shadowPos[1]+=y;
        Vector3 &sPos=shape->SetPos(i);
        sPos.SetFastTransform(iTrans,shadowPos);
      }
      shape->SetClipAll(ClipLandOn|ClipAll);
      if (!ok) return NULL;
        // move destination storage to shadow faces
        shape->SetFaces(dest);
      }
    shadow->SetAutoCenter(false);
    shape->CalculateMinMax();
    shadow->CalculateMinMax(false);
    if( shape->Max().Distance2(shape->Min())>=(100*100) ) shape = NULL;
  }
  return shape;
}

const int ShadowSpecial = OnSurface|IsShadow|IsAlphaFog;

void LODShape::MakeShadow(LODShape *shadow, int level)
{
  if( level<_minShadow || Resolution(level)>900)
  {
    shadow->_lods[level]=NULL;
    return; // this LOD is never used for shadows
  }
  // if shape is already created, there is no need to create it again
  if (shadow->_lods[level])
  {
    return;
  }

  // copy source shape
  Shape *shape = NULL;
  {
    ShapeUsedGeometryLock<> source(this,level);

    if (source->NFaces()>1024 || source->NFaces()==0)
    {
      if (source->NFaces()>1024)
      {
        LogF
        (
          "%s: Too detailed shadow LOD %d:%f (%d faces)",
          (const char *)GetName(),level,Resolution(level),source->NFaces()
        );
      }
      shadow->_lods[level]=NULL;
      return;
    }
    shape = new Shape(*source.GetShape(),false,false,false);
    Assert(!shape->IsVertexBoneRefPresent());
    shadow->_lods[level] = shape;
  }

  if (shape->NFaces()>1024)
  {
    RptF
    (
      "%s: very complex shadow %d (LOD %d:%.3f)",
      (const char *)Name(),
      shape->NFaces(),level,Resolution(level)
    );
    shadow->_lods[level]=NULL;
    return;
  }
  if (shape->NFaces()==0)
  {
    shadow->_lods[level]=NULL;
    return;
  }
  for (int s=0; s<shape->NSections(); s++)
  {
    ShapeSection &ss = shape->GetSection(s);
    if( (ss.Special()&(NoShadow|IsHidden|IsHiddenProxy))==0 )
    {
      if (!(ss.Special()&(IsAlpha|IsTransparent)))
      {
        ss.SetTexture(NULL);
        ss.SetSpecial
        (
          OnSurface|IsShadow|NoZWrite|IsAlphaFog|
          ClampU|ClampV
        );
      }
      else
      {
        #if _ENABLE_PERFLOG
        if (LogStatesOnce)
        {
          LogF("-- Shadow texture %s",(const char *)ss.GetDebugText());
        }
        #endif
        ss.SetSpecial
        (
          OnSurface|IsShadow|NoZWrite|IsAlphaFog|
          ss.Special()&(NoClamp|ClampU|ClampV|::IsAnimated|IsAlpha|IsTransparent)
        );
      }
      ss.SetMaterialExt(NULL); // no materials needed for shadows
    } // if( face has shadow )
  }
  //shape->_face.Resize(dst);
  shape->SetHints(0,0);
  //shape->_face._sections.Clear();
  shape->OrSpecial(ShadowSpecial);
  shape->DisablePushBuffer();
  shape->VertexDeclarationNeedsUpdate();
  shape->UpdateShapePropertiesNeeded();
}

LODShape *LODShape::MakeShadow(int level)
{
  // note shadow shape structure is constant
  // changing are positions of any face
  //
  if( Special()&NoShadow ) return NULL; // no shadow
  // do not copy animation for shadow shapes
  LODShape *shadow=new LODShape(*this,false,false,false,false);
  shadow->DisableStreaming();

  shadow->OrSpecial(ShadowSpecial);
  shadow->_boundingCenter = VZero;

  MakeShadow(shadow,level);

  return shadow;
}
