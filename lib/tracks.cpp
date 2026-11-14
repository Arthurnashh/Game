// Poseidon - vehicle class
// (C) 1997, Suma
#include "wpch.hpp"

#include "tracks.hpp"
#include "global.hpp"
#include "scene.hpp"
#include "landscape.hpp"
#include "engine.hpp"
#include "world.hpp"
#include "vehicleAI.hpp"
#include "txtPreload.hpp"

#include "Shape/specLods.hpp"

DEFINE_FAST_ALLOCATOR(TrackStep)

//const ClipFlags ambFlags = ClipLightAmbient|(ClipUserStep*MSInShadow);
const ClipFlags ambFlags = (ClipUserStep*MSInShadow);
const int TrackSpecFlags=IsAlpha|NoShadow|NoZWrite|IsAlphaFog|IsColored;
const ClipFlags TrackClip=ClipAll|ClipLandOn /*|ClipFogShadow|*/ |ambFlags;

static RStringB TrackString="#track";
#define TrackType VehicleTypes.New(TrackString)

TrackStep::TrackStep
(
  float alpha, Texture *texture
)
:Vehicle(new LODShape,TrackType,CreateObjectId()),
_alpha(alpha),_texture(texture)
{
  Object::_type=TypeTempVehicle;
  // alpha changes Glob.config.invTrackTimeToLive per second
  // each Glob.config.trackTimeToLive changes alpha from 1 to 0
  float timeDelta=Glob.config.trackTimeToLive*0.05;
  SetSimulationPrecision(floatMax(0.1,timeDelta));
  RandomizeSimulationTime();
  SetTimeOffset(floatMax(0.1,timeDelta));

  _startTime=Glob.time; // remember when we were created 
  _shape->SetAutoCenter(false);
  // create all required LOD levels
  float lodCoef=0.71;
  for( int i=0; i<TrackLODLevels; i++ )
  {
    Shape *oShape=new Shape;
    oShape->SetHints(TrackClip,TrackClip);
    _shape->AddShape(oShape,lodCoef);
    lodCoef*=2;
  }
  // set flags
  _shape->OrSpecial(TrackSpecFlags|OnSurface);
  _shape->SetHints(TrackClip,TrackClip);
  //LogF("TrackStep Construct %x",this);
  UpdateAlpha();
}

TrackStep::~TrackStep()
{
  //LogF("TrackStep Destruct %x",this);
}

void TrackStep::Change(
  int level,
  Vector3Par lastLeft, Vector3Par lastRight, float lastV,
  Vector3Par left, Vector3Par right, float v
)
{
  PROFILE_SCOPE_DETAIL_EX(tsChn,track);
  // TODO: check if modification of shape is legal
  ShapeUsed lock = _shape->Level(level);
  Assert(lock.NotNull());
  Shape *shape = lock.InitShape();

  // adapt left and right points of the track
  if( shape->NFaces()<=0 )
  {
    // this is the first face
    // initalize lod level
    shape->ReallocTable(4);
    shape->SetClipAll(TrackClip);
    shape->SetNorm(0)=Vector3Compressed::Up;
    shape->SetNorm(1)=Vector3Compressed::Up;
    shape->SetNorm(2)=Vector3Compressed::Up;
    shape->SetNorm(3)=Vector3Compressed::Up;
    // precalculate hints for possible optimizations
    shape->CalculateHints();
    _shape->CalculateHints();
    // change face parameters
    Poly face;
    face.Init();
    face.SetN(4);
    face.Set(0,1);
    face.Set(1,0);
    face.Set(2,2);
    face.Set(3,3);
    int spec = ClampU|TrackSpecFlags|OnSurface;
    ShapeSectionInfo prop;
    prop.Init();
    prop.SetTexture(_texture);
    //prop.SetMaterialExt(GlobPreloadMaterial(TrackMaterial));
    prop.SetSpecial(spec);
    float size = left.Distance(right);
    // assume whole texture is mapped on the rectangle
    prop.SetAreaOverTex(0, size*size);
    shape->AddFace(face);
    // whole shape should be created by only one section

    shape->OrSpecial(TrackSpecFlags|OnSurface);
    shape->SetHints(TrackClip,TrackClip);
    shape->SetAsOneSection(prop);
  }
  Assert( shape->NVertex()==4 );
  Assert( shape->NFaces()==1 );
  shape->SetPos(0)=PositionWorldToModel(lastLeft);
  shape->SetPos(1)=PositionWorldToModel(lastRight);
  shape->SetPos(2)=PositionWorldToModel(left);
  shape->SetPos(3)=PositionWorldToModel(right);

  // set u,v
  float u0 = 0;
  float u1 = 1;
  
  UVPair minUV(u0, lastV), maxUV(u1, v), invUV;
  if (minUV.v > maxUV.v) Swap(minUV.v, maxUV.v);

  shape->InitUV(invUV, minUV, maxUV);
  shape->SetUV(0, invUV, u0, lastV);
  shape->SetUV(1, invUV, u1, lastV);
  shape->SetUV(2, invUV, u0, v);
  shape->SetUV(3, invUV, u1, v);
  
  Poly &face=shape->FaceIndexed(0);
  Vector3 normal = face.GetNormal(shape->GetPosArray());
  if( normal.Y()>0 )
  {
    // if the face is upside down, mirror it to gurantee upward normal
    int v0=face.GetVertex(0),v1=face.GetVertex(1);
    int v2=face.GetVertex(2),v3=face.GetVertex(3);
    face.Set(0,v1),face.Set(1,v0);
    face.Set(2,v3),face.Set(3,v2);
  }
  _shape->CalculateMinMax(true);
  _shape->ReleaseVBuffers(true);
  // TODO:MC: dynamic buffers not compatible with MC
  _shape->OptimizeRendering(VBDynamic);
}

void TrackStep::Simulate( float deltaT, SimulationImportance prec )
{
  if (!IS_SHADOW_OBJECT) _alpha = 0;
  // disappear a litte
  _alpha-=deltaT*Glob.config.invTrackTimeToLive;
  if( _alpha<=0.05 )
  {
    // the object may be removed
    _alpha=0;
    SetDeleteRaw();
    //LogF("TrackStep delete %x",this);
  }
  UpdateAlpha();
}

void TrackStep::UpdateAlpha()
{
  int alpha=toIntFloor(_alpha*255);
  if( alpha<0 ) alpha=0;
  if( alpha>255 ) alpha=255;
  SetConstantColor(PackedColorRGB(PackedColor(0xffffff),alpha));
}

// no instancing - alpha is used
bool TrackStep::CanBeInstanced(int level, float distance2, const DrawParameters &dp) const
{
  return false;
}

AnimationStyle TrackStep::IsAnimated( int level ) const {return NotAnimated;}

int TrackStep::PassNum( int lod ) {return 1;} // road pass
// draw after roads
int TrackStep::PassOrder( int lod ) const {return 2;}

void TrackStep::Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2)
{
}

void TrackStep::Deanimate( int level, bool setEngineStuff )
{
}

void TrackStep::StartTime()
{
  _startTime=Glob.time;
}

void TrackStep::Final()
{
  for( int level=0; level<_shape->NLevels(); level++ )
  {
    // TODO: check if modification of shape is legal
    ShapeUsed lock = _shape->Level(level);
    Assert(lock.NotNull());
    Shape *shape = lock.InitShape();

    shape->SurfaceSplit(GLOB_LAND,FutureVisualState().Transform(),GLOB_ENGINE->ZShadowEpsilon(),false);
    shape->AndSpecial(~OnSurface);
    shape->OrSpecial(TrackSpecFlags|IsOnSurface);
  }
}

TrackDraw::TrackDraw()
:_offsets(false)
{
  _offsets = false;
  _lOffset = VZero;
  _rOffset = VZero;
  _texture = NULL;
  _alpha = 0.0f;
}

void TrackDraw::SetOffsets
(
  Vector3Par lOffset, Vector3Par rOffset,
  Texture *texture, float alpha
)
{
  _lOffset=lOffset,_rOffset=rOffset;
  _offsets=true;
  _texture=texture;
  _alpha=alpha;
}

TrackStep *TrackDraw::Update( const Frame &pos, bool force )
{
  #if 1
  if (!IS_SHADOW_OBJECT) return NULL;
  if( !_offsets ) return NULL;
  Point3 left=pos.PositionModelToWorld(_lOffset);
  Point3 right=pos.PositionModelToWorld(_rOffset);
  float eps=GLOB_ENGINE->ZShadowEpsilon();
  left[1]=GLOB_LAND->SurfaceY(left[0],left[2])+eps;
  right[1]=GLOB_LAND->SurfaceY(right[0],right[2])+eps;
  const float vMap=2.0;
  for( int level=0; level<TrackLODLevels; level++ )
  {
    const float levelCoef=1<<level;
    const float minStartTrackStep2=Square(0.2*levelCoef);
    const float minFinishTrackStep2=Square(1.0*levelCoef);
    const float maxFinishTrackStep2=Square(4.0*levelCoef);
    const float maxFinishTrackTime=Square(0.25*levelCoef);
    TrackLODLevel &lod=_lods[level];
    if( !lod._initDone )
    {
      lod._lastL=left;
      lod._lastR=right;
      lod._initDone=true;
      //Log("Track Init %d :%f",level,lod._lastV);
      lod._lastV=fastFmod(lod._lastV,1);
    }
    //const float maxTrackStep2=Square(1.0);
    float distL2=lod._lastL.Distance2(left);
    float distR2=lod._lastR.Distance2(right);
    float maxDist2=floatMax(distL2,distR2);
    if( maxDist2>25*25 )
    { // too large step - skip it
      lod._lastL=left;
      lod._lastR=right;
    }
    if( !_lastPart )
    {
      // if we are not building any track we should start now
      if( maxDist2>minStartTrackStep2 || maxDist2>1e-2 && force )
      {
        // always have some polygon ready in the finest level
        _lastPart=new TrackStep(_alpha,_texture);
        _lastPart->SetPosition((lod._lastL+lod._lastR)*0.5);
        GLOB_LAND->AddObject(_lastPart);
        //LogF("AddObject Track %x",_lastPart);
        // make sure object is drawn
        // one ref in landscape, one in this
        Assert (_lastPart->RefCounter()==2);
        // no simulation during construction
      }
      else
      {
        //Log("No track object %d",level);
        continue; // nothing to connect
      }
    }
    //lod._currL=left,lod._currR=right;
    float v=lod._lastV+sqrt((distL2+distR2)*0.5)*vMap;
    //Log("Change track object %d: %f",level,v);
    _lastPart->Change(
      level,
      lod._lastL,lod._lastR,lod._lastV,
      left,right,v
    );
    if
    (
      force ||
      maxDist2>minFinishTrackStep2 && // long enough
      (
        maxDist2>maxFinishTrackStep2 || // too long
        _lastPart->GetStartTime()<Glob.time+maxFinishTrackTime // or too old
      )
    )
    {
      // if the track step is too long disconnect it
      TrackStep *ret=_lastPart;
      // clear all levels but the current - the only to merge
      for( int cl=0; cl<TrackLODLevels; cl++ ) if( cl!=level )
      {
        _lastPart->GetShape()->ChangeShape(cl,new Shape);
      }
      _lastPart->Final();
      if( v>1 && force ) lod._lastV=fastFmod(v,1);
      else lod._lastV=v;
      _lastPart.Free();
      // one ref in landscape
      lod._lastL=left,lod._lastR=right;
      //Log("Finish track object %d: %f (forced %d)",level,v,force);
      return ret;
    }
    else
    {
      /*
      LODShape *lShape = _lastPart->GetShape();
      for( int level=0; level<lShape->NLevels(); level++ )
      {
        Shape *shape=lShape->Level(level);
        shape->FindSections();
      }
      */
    }
  }
  #endif
  return NULL;
}

void TrackDraw::Skip( const Frame &pos )
{
  if( !_offsets ) return;
  Point3 left=pos.PositionModelToWorld(_lOffset);
  Point3 right=pos.PositionModelToWorld(_rOffset);
  float eps=GLOB_ENGINE->ZShadowEpsilon();
  left[1]=GLOB_LAND->SurfaceY(left[0],left[2])+eps;
  right[1]=GLOB_LAND->SurfaceY(right[0],right[2])+eps;
  const float vMap=2.0;
  for( int level=0; level<TrackLODLevels; level++ )
  {
    TrackLODLevel &lod=_lods[level];
    if( lod._initDone )
    {
      float distL2=lod._lastL.Distance2(left);
      float distR2=lod._lastR.Distance2(right);
      float v=lod._lastV+sqrt((distL2+distR2)*0.5)*vMap;
      lod._lastV=v;
      lod._lastL=left;
      lod._lastR=right;
      //Log("Skip track object %d: %f",level,v);
    }
  }
}

void TrackAccumulator::Terminate()
{
  if( _accumulate )
  {
    // before termination finish all lod levels
    
    LODShape *lShape=_accumulate->GetShape();
    for( int level=0; level<TrackLODLevels; level++ )
    {
      // TODO: check if modification of shape is legal
      ShapeUsed lock = lShape->Level(level);
      Assert(lock.NotNull());
      Shape *shape = lock.InitShape();

      shape->Compact();
    }
    
    // when shape is closed, convert to v-buffer
    lShape->ReleaseVBuffers(true);
    lShape->OptimizeRendering(VBSmallDiscardable);


    // remmember it on dissapear list
    // Ref to _accumulate should be maintained in landscape 
    Assert( _accumulate->RefCounter()>1 );  
    // start fading out - add to simulated vehicle list
    _accumulate->StartTime();
    //LogF("RemoveObject Acc Track %x",_accumulate);
    GLOB_LAND->RemoveObject(_accumulate);
    GLOB_WORLD->AddAnimal(_accumulate);
    //LogF("AddAnimal Acc Track %x",_accumulate);
    _accumulate.Free();
    //Log("Terminate track object");
  }
}

/*!
  \patch 5142 Date 03/19/2007 by Flyman
  - Fixed: Tracks were drawn under the bridge
*/
bool TrackAccumulator::ShouldTerminate(const Frame &pos, Vector3Par avgTrackPos, bool terminate) const
{
  // Determine whether model is upside down (we will not draw tracks in such case)
  bool upsideDown = (pos.DirectionUp().Y() < 0.3f);

  // Determine whether model is on a bridge (we will not draw tracks in such case as they would be drawn on land surface)
  bool onBridge = false;
  {
    // Get the avg track position
    Vector3 atp = pos.PositionModelToWorld(avgTrackPos);

    // If road surface is above the land surface, then finish
    const float maxRSOffset = 0.01f;
    const float offsetY = 0.1f;
    if (GLOB_LAND->RoadSurfaceY(atp + Vector3(0, offsetY, 0)) > GLOB_LAND->SurfaceY(atp[0], atp[2]) + maxRSOffset) onBridge = true;
  }

  // Update the terminate status
  return terminate || upsideDown || onBridge;
}

bool TrackAccumulator::Merge( TrackStep *with )
{
  if( !_accumulate )
  {
    LODShape *lShapeWith=with->GetShape();

    lShapeWith->OptimizeRendering(VBDynamic);
    lShapeWith->ReleaseVBuffers(true);
    _accumulate=with;
  }
  else
  {
    // merge with accumulate
    LODShape *lShape=_accumulate->GetShape();
    if( with )
    {
      LODShape *lShapeWith=with->GetShape();
      // transform merged shape to accumulate coord space 
      Matrix4Val withToAccum=_accumulate->InvTransform()*with->FutureVisualState().Transform();
      for( int level=0; level<lShape->NLevels(); level++ )
      {
        // TODO: check if modification of shape is legal
        ShapeUsed lock = lShape->Level(level);
        Assert(lock.NotNull());
        Shape *shape = lock.InitShape();

        ShapeUsed shapeWith = lShapeWith->Level(level);
        Assert( shapeWith.NotNull() );
        // reserve enough space to avoid future reallocations
        shape->Reserve(64);
        shape->ReserveFaces(32);

        // actually merge shapes and remove with object from landscape
        shape->Merge(shapeWith.GetShape(),withToAccum);
        shape->CalculateHints();
      }

      GLOB_LAND->RemoveObject(with);
      //LogF("RemoveObject Merge Track %x",with);
    }
    lShape->SetAutoCenter(true);
    Point3 oldCenter=lShape->BoundingCenter();
    lShape->CalculateHints();
    lShape->CalculateMinMax(true);
    // any previous buffers are invalid now
    lShape->ReleaseVBuffers(true);
    lShape->OptimizeRendering(VBDynamic);

    //LogF("RemoveObject Acc Track %x",_accumulate);
    GLOB_LAND->RemoveObject(_accumulate);
    _accumulate->SetPosition
    (
      _accumulate->FutureVisualState().PositionModelToWorld(lShape->BoundingCenter()-oldCenter)
    );
    GLOB_LAND->AddObject(_accumulate);
    //LogF("AddObject Acc Track %x",_accumulate);
    //with->SetTransform(_accumulate->Transform());
    // if accumulate is to large or too old discontinue it
    float howOld=Glob.time-_accumulate->GetStartTime();
    if( lShape->BoundingSphere()>=20.0 || howOld>=Glob.config.trackTimeToLive*0.1 )
    {
      //Log("Disconnect track object");
      return true;
    }
  }
  return false;
}

TrackOptimizedFour::TrackOptimizedFour(const LODShape *lShape, ParamEntryPar par)
{
  float alpha = par >> "alphaTracks";
  RStringB name = par >> "memoryPointTrackFLL";
  if (lShape->MemoryPointExists(name))
  {
    _fLeft.SetOffsets
    (
      lShape->MemoryPoint((par >> "memoryPointTrackFLR").operator RString()), lShape->MemoryPoint(name),
      GLOB_SCENE->Preloaded(TrackTextureFour),alpha
    );
  }
  name = par >> "memoryPointTrackBLL";
  if (lShape->MemoryPointExists(name))
  {
    _bLeft.SetOffsets
    (
      lShape->MemoryPoint((par >> "memoryPointTrackBLR").operator RString()),lShape->MemoryPoint(name),
      GLOB_SCENE->Preloaded(TrackTextureFour),alpha
    );
  }
  name = par >> "memoryPointTrackFRL";
  if (lShape->MemoryPointExists(name))
  {
    _fRight.SetOffsets
    (
      lShape->MemoryPoint((par >> "memoryPointTrackFRR").operator RString()),lShape->MemoryPoint(name),
      GLOB_SCENE->Preloaded(TrackTextureFour),alpha
    );
  }
  name = par >> "memoryPointTrackBRL";
  if (lShape->MemoryPointExists(name))
  {
    _bRight.SetOffsets
    (
      lShape->MemoryPoint((par >> "memoryPointTrackBRR").operator RString()),lShape->MemoryPoint(name),
      GLOB_SCENE->Preloaded(TrackTextureFour),alpha
    );
  }
}

void TrackOptimizedFour::Update( const Frame &pos, float deltaT, bool terminate )
{
  // TODO: optimize
  if (!ShouldTerminate(pos, (BackLeftPos() + BackRightPos() + FrontLeftPos() + FrontRightPos()) * 1.0f / 4.0f, terminate))
  {
    bool disconnect=false;
    TrackStep *merge;
    if( !disconnect && (merge=_fLeft.Update(pos,false))!=NULL ) disconnect=Merge(merge);
    if( !disconnect && (merge=_bLeft.Update(pos,false))!=NULL ) disconnect=Merge(merge);
    if( !disconnect && (merge=_fRight.Update(pos,false))!=NULL ) disconnect=Merge(merge);
    if( !disconnect && (merge=_bRight.Update(pos,false))!=NULL ) disconnect=Merge(merge);
    if( disconnect )
    {
      if( merge=_fLeft.Update(pos,true) ) Merge(merge);
      if( merge=_fRight.Update(pos,true) ) Merge(merge);
      if( merge=_bLeft.Update(pos,true) ) Merge(merge);
      if( merge=_bRight.Update(pos,true) ) Merge(merge);
      Terminate();
    }
  }
  else
  {
    _fLeft.Skip(pos);
    _bLeft.Skip(pos);
    _fRight.Skip(pos);
    _bRight.Skip(pos);
    Terminate();
  }
}

TrackOptimized::TrackOptimized(const LODShape *lShape, ParamEntryPar par)
{
  float alpha = par >> "alphaTracks";
  bool wheelTexture = par >> "textureTrackWheel";
  Texture *texture = wheelTexture ? GScene->Preloaded(TrackTextureFour) : GScene->Preloaded(TrackTexture);

  RString name = par >> "memoryPointTrack1L";
  if (lShape->MemoryPointExists(name))
  {
    _left.SetOffsets
    (
      lShape->MemoryPoint((par >> "memoryPointTrack1R").operator RString()),lShape->MemoryPoint(name),
      texture, alpha
    );
  }
  name = par >> "memoryPointTrack2L";
  if (lShape->MemoryPointExists(name))
  {
    _right.SetOffsets
    (
      lShape->MemoryPoint((par >> "memoryPointTrack2R").operator RString()),lShape->MemoryPoint(name),
      texture, alpha
    );
  }
}

void TrackOptimized::Update( const Frame &pos, float deltaT, bool terminate )
{
  // TODO: optimize
  if (!ShouldTerminate(pos, (LeftPos() + RightPos()) * 1.0f / 2.0f, terminate))
  {
    bool disconnect=false;
    TrackStep *merge;
    if( !disconnect && (merge=_left.Update(pos,false))!=NULL ) disconnect=Merge(merge);
    if( !disconnect && (merge=_right.Update(pos,false))!=NULL ) disconnect=Merge(merge);
    if( disconnect )
    {
      if( merge=_left.Update(pos,true) ) Merge(merge);
      if( merge=_right.Update(pos,true) ) Merge(merge);
      Terminate();
    }
  }
  else
  {
    _left.Skip(pos);
    _right.Skip(pos);
    Terminate();
  }
}

DEFINE_FAST_ALLOCATOR(Mark)

Mark::Mark( LODShape *shape, float alpha, float timeToLive )
:base(shape,GWorld->Preloaded(ETypeMark),CreateObjectId())
{
  Object::_type=TypeTempVehicle;
  SetSimulationPrecision(0.5f); // twice per second is enough
  SetTimeOffset(0.5f);

  RandomizeSimulationTime();
  _shape->SetAutoCenter(false);
  _shape->OrSpecial(TrackSpecFlags|OnSurface);
  for (int i=0; i<_shape->NLevels(); i++)
  {
    // TODO: check if modification of shape is legal
    ShapeUsed lock = _shape->Level(i);
    Assert(lock.NotNull());
    Shape *shape = lock.InitShape();

    shape->SetClipAll(TrackClip);
    shape->SetHints(TrackClip,TrackClip);
  }
  _shape->SetHints(TrackClip,TrackClip);
  _alpha=alpha;
  _alphaSpeed=alpha/timeToLive;
  UpdateAlpha();
}

Mark::~Mark()
{
}

float Mark::DrawingAreaRatio() const
{
  /// with typical camera drawing area will be much lesses
  return 0.02f;
}

void Mark::UpdateAlpha()
{
  int alpha=toIntFloor(_alpha*255);
  if( alpha<0 ) alpha=0;
  if( alpha>255 ) alpha=255;
  SetConstantColor(PackedColorRGB(PackedColor(0xffffff),alpha));
}

void Mark::Simulate( float deltaT, SimulationImportance prec )
{ 
  if (!IS_SHADOW_OBJECT) _alpha = 0;

  _alpha-=deltaT*_alphaSpeed;
  if( _alpha<=0 )
  {
    _alpha=0;
    SetDelete();
  }
  UpdateAlpha();
}

bool Mark::CanBeInstanced(int level, float distance2, const DrawParameters &dp) const {return false;}
AnimationStyle Mark::IsAnimated( int level ) const {return NotAnimated;}

int Mark::PassNum( int lod ) {return 1;} // road pass
// draw after roads
int Mark::PassOrder( int lod ) const {return 2;}

void Mark::Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2)
{
}

void Mark::Deanimate( int level, bool setEngineStuff )
{
}
