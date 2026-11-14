// sounds for objects

#include "wpch.hpp"
#include "vehicle.hpp"
#include "objectClasses.hpp"
#include "AI/ai.hpp"
#include "lights.hpp"
#include "engine.hpp"
#include "landscape.hpp"
#include "scene.hpp"
#include "camera.hpp"
#include <El/Common/randomGen.hpp>
#include "global.hpp"
#include "fileLocator.hpp"
#include "objDestroy.hpp"
#include "rtAnimation.hpp"
#include "tlVertex.hpp" // because of VASI point light rendering

static const EnumName LightStateNames[]=
{
  EnumName(StreetLamp::LSOff, "OFF"),
  EnumName(StreetLamp::LSOn, "ON"),
  EnumName(StreetLamp::LSAuto, "AUTO"),
  EnumName()
};
template<>
const EnumName *GetEnumNames(StreetLamp::LightState dummy)
{
  return LightStateNames;
}

StreetLampType::StreetLampType( ParamEntryPar param )
:base(param)
{
  _bulbHit = -1;
}

StreetLampType::~StreetLampType()
{
}

void StreetLampType::Load(ParamEntryPar par, const char *shape)
{
  base::Load(par,shape);

  _colorDiffuse = GetColor(par>>"colorDiffuse");
  _colorAmbient = GetColor(par>>"colorAmbient");
  _radius = par>>"radius";
}

void StreetLampType::InitShape()
{
  base::InitShape();
  _bulbHit = FindHitPoint("HitBulb");
}

void StreetLampType::DeinitShape()
{
  base::DeinitShape();
}


Object* StreetLampType::CreateObject(bool unused) const
{ 
  return new StreetLamp(GetShape(), this, VISITOR_NO_ID);
}


DEFINE_CASTING(StreetLamp)

StreetLamp::StreetLamp( LODShape *shape, const StreetLampType *type, const CreateObjectId &id )
:base(shape,type,id),
_pilotLight(true), 
_lightPos(shape->MemoryPoint("light"))
{
  SetSimulationPrecision(12.1256);
  RandomizeSimulationTime();
  SetTimeOffset(0);

  _destrType=DestructTree;
  _static=true;
  Object::_type=Primary;
  _lightState = LSAuto;
}

static const Color StreetLightColor(0.9,0.8,0.6);
static const Color StreetLightAmbient(0.1,0.1,0.1);

void StreetLamp::SwitchLight(LightState state)
{
  _lightState = state;
  SimulateSwitch();
  // check if coordinates make sense
  if (FutureVisualState().Position().SquareSize()>100)
  {
    CreateLight(GetFrameBase());
  }
  else
  {
#if !_VBS3 // switchAllLights
    Fail("Switched lamp not in landscape");
#endif
  }
}

void StreetLamp::Init( Matrix4Par pos, bool init )
{
  base::Init(pos, init);
  SimulateSwitch();
  CreateLight(pos);
}

void StreetLamp::CreateLight(Matrix4Par pos)
{
  if( !_light )
  {
    if( _pilotLight )
    {
      Ref<LODShape> shape=GLOB_SCENE->Preloaded(HalfLight);
      _light=new LightReflector(shape, Type()->_colorDiffuse, Type()->_colorAmbient, 150.0f, 0.5f, false, 1.0f);
			_light->SetRadius(50.0f);
      _light->SetPosition(pos.FastTransform(_lightPos));
      _light->SetDirectionAndUp(Vector3(0,-1,0), Vector3(0,0,1));
      GLOB_SCENE->AddLight(_light);
    }
  }
  else
  {
    if( !_pilotLight )
    {
      _light.Free();
    }
  } 
}

void StreetLamp::SimulateSwitch()
{
  if( GetTotalDamage()<0.3 && GetHit(Type()->_bulbHit)<0.5f)
  {
    switch (_lightState)
    {
    case LSOff:
      _pilotLight = false;
      break;
    case LSOn:
      _pilotLight = true;
      break;
    default:
      Fail("Light state");
    case LSAuto:
      {
        float timeOfDay = Glob.clockWeather.GetTimeOfDay();
        _pilotLight = (timeOfDay < 0.3 || timeOfDay > 0.7);
      }
      break;
    }
  }
  else
  {
    _pilotLight=false;
  } 
}

void StreetLamp::ResetStatus()
{
  _lightState = LSAuto;

  // force update as soon as possible
  _simulationSkipped = _simulationPrecision;
  base::ResetStatus();
}

void StreetLamp::HitBy( EntityAI *killer, float howMuch, RString ammo, bool wasDestroyed )
{
  SimulateSwitch();
  CreateLight(GetFrameBase());
}

void StreetLamp::OnTimeSkipped()
{
  SimulateSwitch();
  CreateLight(GetFrameBase());
}

bool StreetLamp::SimulationReady(SimulationImportance prec) const
{
  // street lamp simulation is always ready
  return true;
}

void StreetLamp::Simulate( float deltaT, SimulationImportance prec )
{
  SimulateSwitch();
  CreateLight(GetFrameBase());
  //if( !_object ) _delete=true;
}

/*!
\patch 1.35 Date 12/10/2001 by Ondra
- Fixed: Street lamp switch was not saved - lamp was off after loading.
\patch 1.57 Date 5/16/2002 by Ondra
- Fixed: Switching street lamps took often too long.
This was especially visible when starting mission.
*/


LSError StreetLamp::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar))
  CHECK(ar.SerializeEnum("lightState", _lightState, 1, LSAuto))
  CHECK(ar.Serialize("pilotLight", _pilotLight, 1, false))
  if (ar.IsLoading())
  {
    SimulateSwitch();
    CreateLight(GetFrameBase());
  }
  return LSOK;
}

//////
VASILightsType::VASILightsType( ParamEntryPar param )
:base(param), _enabled(true)
{
}

VASILightsType::~VASILightsType()
{
}

void VASILightsType::Load(ParamEntryPar par, const char *shape)
{
  base::Load(par,shape);

  _slope = par>>"vasiSlope";
	_enabled = par.GetBoolDef("enabled", true);
}

void VASILightsType::InitShape()
{
  base::InitShape();
  
  const ParamEntry &par = *_par;

  _white.Init(_shape, (par >> "selectionWhiteLight").operator RString(), NULL);
  _red.Init(_shape, (par >> "selectionRedLight").operator RString(), NULL);
  _off.Init(_shape, (par >> "selectionOffLight").operator RString(),NULL);
}
void VASILightsType::DeinitShape()
{
  base::DeinitShape();
}


void VASILightsType::InitShapeLevel(int level)
{
  base::InitShapeLevel(level);
  _white.InitLevel(_shape,level);
  _red.InitLevel(_shape,level);
  _off.InitLevel(_shape,level);
}
void VASILightsType::DeinitShapeLevel(int level)
{
  _white.DeinitLevel(_shape,level);
  _red.DeinitLevel(_shape,level);
  _off.DeinitLevel(_shape,level);
  base::DeinitShapeLevel(level);
}


Object* VASILightsType::CreateObject(bool unused) const
{ 
  return new VASILights(this, VISITOR_NO_ID);
}


VASILights::VASILights(const EntityAIType *type, const CreateObjectId &id )
:base(type)
{
  Assert(dynamic_cast<const VASILightsType *>(type));
  _destrType=GetType()->GetDestructType();
  if( _destrType==DestructDefault ) _destrType=DestructBuilding;
  
  SetSimulationPrecision(15.489);
  RandomizeSimulationTime();
  SetTimeOffset(0);

  _static=true;
  SetType(Primary);
  SetID(id);

  // Create the point light object, shade the shape with the VASI (it doesn't matter what shape will be there - just NULL can't be there)
  _pointLight = new PointLight(GLandscape->GetPointShape(), VISITOR_NO_ID);
}

void VASILights::Init( Matrix4Par pos, bool init )
{
  base::Init(pos, init);
}

void VASILights::ResetStatus()
{
  base::ResetStatus();
}

void VASILights::HitBy( EntityAI *killer, float howMuch, RString ammo, bool wasDestroyed )
{
  base::HitBy(killer,howMuch,ammo,wasDestroyed);
}

bool VASILights::SimulationReady(SimulationImportance prec) const
{
  // light simulation is always ready
  return true;
}

/*
const float VASIExtendVisibility = 1.0f;
const float VASIExtendPixelSize = 100.0f;
const float VASIExtendDensity = 4.0f;
*/
static float VASIExtendVisibility = 1.0f;
static float VASIExtendPixelSize = 400.0f;
static float VASIExtendDensity = 4.0f;

float VASILights::EstimateArea() const
{
  // used when creating object groups
  return base::EstimateArea()*Square(VASIExtendVisibility*VASIExtendPixelSize);
}

float VASILights::MinPixelArea(float minPixelAuto) const
{
  // VASI lights are especially important to be rendered
  // (we rather want to see a flickering light than not see it at all)
  //return floatMin(0.1f,minPixelAuto*0.1f);
  return Square(1.0f/VASIExtendPixelSize);
}

float VASILights::DensityRatio() const
{
  // even when rendering, we do not need to get much more polygons than appropriate
  return base::DensityRatio()*(VASIExtendDensity/Square(VASIExtendVisibility));
}

float VASILights::DrawingAreaRatio() const
{
  // used when rendering
  return base::DrawingAreaRatio()*Square(VASIExtendVisibility);
}

/// reduce the value so that it is closer to zero

static inline float ReduceAbs(float value, float reduce)
{
  if (fabs(value)<reduce) return 0;
  return value - fSign(value)*reduce;
}

enum VASIColor
{
  /// no light visible
  VASINone,
  /// while light visible
  VASIWhite,
  /// red light visible
  VASIRed,
};

// lights visible only when in certain horizontal/vertical angle
static VASIColor CheckVASIColor(Vector3Par camModelPos, float slope, float xDim, float yDim)
{
  if (camModelPos.Z()<0)
  {
    // if light area is big, make sure the light from the whole area is visible under given angle
    Vector3 anglePos(ReduceAbs(camModelPos.X(),xDim),ReduceAbs(camModelPos.Y(),yDim),camModelPos.Z());
    // check angle from the ILS beam
    Vector3 beam(0,slope,-1);
    float cosAngle = beam.CosAngle(anglePos);
    const float cosMaxAngle = 0.96592582629f; // cos (15 deg)
    //const float cosMaxAngle = 0.98480775301f; // cos (10 deg)
    if (cosAngle>cosMaxAngle)
    {
      // check if red or white is visible
      if (camModelPos.Y()<-camModelPos.Z()*slope)
      {
        return VASIRed;
      }
      else
      {
        return VASIWhite;
      }
    }
  }
  return VASINone;
}

/**
 this is the main reason for this class to exist
 Lights are animated depending on camera position
*/
void VASILights::Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2)
{
  base::Animate(animContext, level, setEngineStuff, parentObject, dist2);

  // convert camera position to model space
  
  if (!GScene) return;
  Vector3 camModelPos = animContext.GetVisualState().cast<VASILights>().PositionWorldToModel(GScene->GetCamera()->Position());
  //VASIColor color = CheckVASIColor(camModelPos,Type()->_slope,_shape->MinMax()[1].X(),_shape->MinMax()[1].Y());
  VASIColor color = VASINone;
  
  if (color==VASIWhite) Type()->_white.Unhide(animContext, _shape, level);
  else Type()->_white.Hide(animContext, _shape, level);
  if (color==VASIRed) Type()->_red.Unhide(animContext, _shape, level);
  else Type()->_red.Hide(animContext, _shape, level);
  if (color==VASINone) Type()->_off.Unhide(animContext, _shape, level);
  else Type()->_off.Hide(animContext, _shape, level);
}

void VASILights::Deanimate(int level, bool setEngineStuff)
{
  base::Deanimate(level,setEngineStuff);
}

void VASILights::Simulate( float deltaT, SimulationImportance prec )
{
  base::Simulate(deltaT,prec);
}

/*!
\patch 5162 Date 6/1/2007 by Ondra
- Fixed: VASI light rendering distance significantly increased.
*/

int VASILights::PassNum( int lod )
{
  if (lod==_shape->FindSimplestLevel())
  {
    return 2; // alpha needed
  }
  int pass = base::PassNum(lod);
  // at least partial alpha needed
  if (pass<0) pass = 0;
  return pass;
}

void VASILights::PrepareProxiesForDrawing(
  Object *rootObj, int rootLevel, const AnimationContext &animContext,
  const PositionRender &transform,int level, ClipFlags clipFlags, SortObject *so)
{
  // drawing the last level - pick one point and draw as a point as well
  // select color based on a position
  if (!GScene || !Type()->_enabled) 
		return;

  Vector3 camModelPos = animContext.GetVisualState().cast<VASILights>().PositionWorldToModel(GScene->GetCamera()->Position());
  VASIColor color = CheckVASIColor(camModelPos,Type()->_slope,_shape->MinMax()[1].X(),_shape->MinMax()[1].Y());
  if (color!=VASINone)
  {
    float dist2 = so->distance2;
    
    // some light visible - create a color and draw it
    Color pointColor = color==VASIRed ? Color(0.933*3.42,0.02*3.42,0.02*3.42) : Color(1,1,1);

    // Get the light intensity - three step VASI system
    float intensity;
    #if 0
    { // three state system, as described in VASI Intensity Setting, Three-Step System
      // http://www.faa.gov/air_traffic/publications/atpubs/FSS/fss1101.htm 11-1-18.
      static float dayIntensity = 1800000.0f;
      static float twilightIntensity = 100000.0f;
      static float nightIntensity = 500.0f;
      static float sunYSwitch1 = -0.20f;
      static float sunYSwitch2 = -0.014f;

      // Calculate the final intensity according to a sun position
      float sunY = GScene->MainLight()->SunDirection().Y();
      if (sunY < sunYSwitch1)
      {
        intensity = dayIntensity;
      }
      else if (sunY < sunYSwitch2)
      {
        intensity = twilightIntensity;
      }
      else
      {
        intensity = nightIntensity;
      }
    }
    #else
    {
      // photoelectric intensity control, 
      static float maxIntensity = 15000000.0f;
      static float minIntensity = 500.0f;
      static float diffCoef = 0.5f;
      static float relIntensity = 5000.0f;
      LightSun *sun = GScene->MainLight();
      float sunIntensity = sun->GetAmbient().Brightness()+sun->GetDiffuse().Brightness()*diffCoef;
      intensity = floatMinMax(relIntensity*sunIntensity,minIntensity,maxIntensity);
    }
    #endif

    // Set color and intensity
    _pointLight->SetColorAndIntensity(pointColor, intensity / dist2);

    // Prepare point to be drawn
    GScene->ProxyForDrawing(_pointLight, rootObj, 0, rootLevel, clipFlags, dist2, transform);
  }
}



LSError VASILights::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar))
  return LSOK;
}

DEFINE_FAST_ALLOCATOR(ForestPlain)

DEFINE_CASTING(ForestPlain)


ForestPlain::ForestPlain( LODShape *shape, const CreateObjectId &id )
:base(shape,id)
{
  _skewMatrix = SkewNone;
  // check - some forest may use single matrix
  const char *name = shape->Name();
  if (strstr(name,"_t1.p3d") )
  {
    _skewMatrix = SkewT1;
  }
  else if ( strstr(name,"_t2.p3d"))
  {
    _skewMatrix = SkewT2;
  }
  else if (strstr(name,"_1.p3d") )
  {
    _skewMatrix = SkewPos;
  }
  else if ( strstr(name,"_2.p3d"))
  {
    _skewMatrix = SkewPos;
  }
  #if _ENABLE_REPORT
  if (_skewMatrix==SkewNone && GUseFileBanks)
  {
    // this situation is currently quite normal during binarize
    RptF("Forest %s requires CPU animation",name);
  }
  #endif
  if (_skewMatrix!=SkewNone)
  {
    shape->SetAnimationType(AnimTypeNone);
  }
  shape->OptimizeRendering(VBStatic);
}

AnimationStyle ForestPlain::IsAnimated( int level ) const
{
  // if there is no specific skew handling, some CPU work will be needed
  return _skewMatrix==SkewNone ? AnimatedGeometry : NotAnimated;
}

void ForestPlain::InitSkew(Landscape *land)
{
  /*
  // forest skew is handled using InitSkewReplace
  // check what is current skew in the transformation matrix
  Matrix4 trans = GetFrameBase();
  trans.SetOrientation(M3Identity);
  InitSkewReplace(land,trans);
  
  if (
    fabs(Orientation()(1,0))>0.1f ||
    fabs(Orientation()(1,2))>0.1f 
  )
  {
    __asm nop;
  }
  // check if skew matches
  static bool fixSkew = false;
  if (fixSkew)
  {
    SetOrientation(trans.Orientation());
  }
  */
}

void ForestPlain::DeSkew(const Landscape *land, Matrix4& trans, float& relHeight)
{
}

bool ForestPlain::InitSkewReplace(Landscape *land, Matrix4 &trans)
{
  if (_skewMatrix==SkewNone) return false;

  float xC=trans.Position().X();
  float zC=trans.Position().Z();
  // fine rectangles are not used - use rough instead
  // calculate surface level on given coordinates
  float xRel=xC*InvLandGrid;
  float zRel=zC*InvLandGrid;

  int x=toIntFloor(xRel);
  int z=toIntFloor(zRel);

  int subdivLog = land->GetSubdivLog();
  int subdiv = 1<<subdivLog;

  int xs=x*subdiv;
  int zs=z*subdiv;

  float y00 = land->GetHeight(zs,xs);
  float y01 = land->GetHeight(zs,xs+subdiv);
  float y10 = land->GetHeight(zs+subdiv,xs);
  float y11 = land->GetHeight(zs+subdiv,xs+subdiv); 

  float d1000 = y10-y00;
  float d0100 = y01-y00;

  float d1011 = y10-y11;
  float d0111 = y01-y11;

  float skewX = 0,skewZ = 0;

  if (_skewMatrix==SkewPos)
  {
    // world space center of the objects
    // bounding center of the forest is always near the grid center
    // we need to find another point 
    Vector3 wCenter = trans.Position(); //.FastTransform(-_shape->BoundingCenter());
    // check where it is in the square xC,zC
    float xGrid = wCenter.X()-x*LandGrid;
    float zGrid = wCenter.Z()-z*LandGrid;
    // skew matrix should be based on the object center position
    Assert(xGrid>=-0.1 && xGrid<=LandGrid+0.1);
    Assert(zGrid>=-0.1 && zGrid<=LandGrid+0.1);
    if( xGrid<=LandGrid-zGrid)
    {
      _skewMatrix = SkewT2;
    }
    else
    {
      _skewMatrix = SkewT1;
    }
  }
  
  if (_skewMatrix==SkewT2)
  {
    // create a skew matrix for T2
    // T1 dy y00+d1000*zIn+d0100*xIn :
    skewX = d0100*InvLandGrid;
    skewZ = d1000*InvLandGrid;
  }
  else if (_skewMatrix==SkewT1)
  {
    // T2 dy y10+d0111-d1011*xIn-zIn*d0111
    // create a skew matrix for T1
    skewX = -d1011*InvLandGrid;
    skewZ = -d0111*InvLandGrid;
  }
  else
  {
    Fail("Uknown skew");
  }

  // note: object may be rotated
  // calculate (0,0,0) current height
  // model coordinates of original (0,0,0) point are -_shape->BoundingCenter()

  // apply skew settings
  Matrix3 skewedOrient = M3Identity;
  skewedOrient(1,0) = skewX;
  skewedOrient(1,2) = skewZ;

  skewedOrient = skewedOrient * trans.Orientation();
  
  // calculate where will be the bounding center transformed
  Vector3 bcW = skewedOrient * -_shape->BoundingCenter() + trans.Position();
  
  // calculate surface position using current plane equation
  float xIn=bcW.X()*InvLandGrid-x; // relative 0..1 in square
  float zIn=bcW.Z()*InvLandGrid-z;
  float y=
  (
    _skewMatrix==SkewT2 ? y00+d1000*zIn+d0100*xIn :
    y10+d0111-d1011*xIn-zIn*d0111
  );
  
  float offsetY = y-bcW.Y();

  trans.SetPosition(Vector3(0,offsetY,0)+trans.Position());
  trans.SetOrientation(skewedOrient);
  return true;
}

float ForestPlain::ViewDensity() const
{
  /// log(0.3)/50  - 0.3 visibility for 50 m distance
  return -0.024079456087f;
}

#pragma optimize("t",on)
#if !_RELEASE
  // it seems in VS 2005 optimize("t",on) applies /Oy as well (Omit frame pointers)
  // in Testing we need stack frames for memory call-stack and crash-dumps
  #pragma optimize("y",off)
#endif
void ForestPlain::Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2)
{
  ShapeUsed shape=_shape->Level(level);
  if( shape.IsNull() ) return;

  Assert( GLandscape );
  // save original position

  if (_skewMatrix==SkewNone) 
  {
    // matrix not included in object matrix - skew
    // TODO: get plane equation for parts A/B

    float xC=FutureVisualState().Position().X();
    float zC=FutureVisualState().Position().Z();
    // fine rectangles are not used - use rough instead
    // calculate surface level on given coordinates
    float xRel=xC*InvLandGrid;
    float zRel=zC*InvLandGrid;
    int x=toIntFloor(xRel);
    int z=toIntFloor(zRel);
    float xf=x;
    float zf=z;

    int subdivLog = TerrainRangeLog-LandRangeLog;
    int subdiv = 1<<subdivLog;

    int xs=x*subdiv;
    int zs=z*subdiv;

    Landscape *land=GLandscape;
    float y00=land->GetHeight(zs,xs);
    float y01=land->GetHeight(zs,xs+subdiv);
    float y10=land->GetHeight(zs+subdiv,xs);
    float y11=land->GetHeight(zs+subdiv,xs+subdiv);

    float d1000=y10-y00;
    float d0100=y01-y00;

    float d1011=y10-y11;
    float d0111=y01-y11;

    animContext.InvalidateNormals(shape);

    Matrix4Val toWorld=FutureVisualState().Transform();
    Matrix4Val fromWorld=FutureVisualState().GetInvTransform();

    // convert plane equations to model space
    // change object shape to reflect surface
    // most forest objects are not rotated, only offseted - this can be easily optimized
    bool rotated=FutureVisualState().Direction()*VForward<0.99;
    float yOffset=-_shape->BoundingCenter().Y();
    if( !rotated )
    {
      for( int i=0; i<shape->NPos(); i++ )
      {
        Vector3Val pos=shape->Pos(i);
        // shape y is relative to surface
        // calculate world coordinates
        float yPos=pos[1]-yOffset;
        Vector3 tPos=pos+FutureVisualState().Position();

        float xIn=tPos.X()*InvLandGrid-xf; // relative 0..1 in square
        float zIn=tPos.Z()*InvLandGrid-zf;
        float y=
        (
          xIn<=1-zIn ? y00+d1000*zIn+d0100*xIn :
          y10+d0111-d1011*xIn-zIn*d0111
        );

        Vector3 &dPos = animContext.SetPos(shape, i);
        dPos[1]=y+yPos-FutureVisualState().Position().Y();
      }

    }
    else
    {
      for( int i=0; i<shape->NPos(); i++ )
      {
        Vector3Val pos=shape->Pos(i);
        // shape y is relative to surface
        // calculate world coordinates
        Vector3 tPos(VFastTransform,toWorld,pos);
        float xIn=tPos.X()*InvLandGrid-xf; // relative 0..1 in square
        float zIn=tPos.Z()*InvLandGrid-zf;

        float yPos=pos[1]-yOffset;
        float y=
        (
          xIn<=1-zIn ? y00+d1000*zIn+d0100*xIn :
          y10+d0111-d1011*xIn-zIn*d0111
        );
        tPos[1]=y+yPos;

        Vector3 &dPos = animContext.SetPos(shape, i);
        dPos.SetFastTransform(fromWorld,tPos);
      }
    }

    
    // animate bounding box
    // i.e. animate all 8 bbox corners
    Vector3 minValue(1e10,1e10,1e10),maxValue(-1e10,-1e10,-1e10);
    for( int lr=0; lr<2; lr++ )
    for( int ud=0; ud<2; ud++ )
    for( int fb=0; fb<2; fb++ )
    {
      // assume generic (rotated) case here
      Vector3 pos = shape->MinMaxCorner(lr,ud,fb);
      // corner is source model coordinates
      // convert to animated coordinates
      Vector3 tPos(VFastTransform,toWorld,pos);
      float xIn=tPos.X()*InvLandGrid-xf; // relative 0..1 in square
      float zIn=tPos.Z()*InvLandGrid-zf;
      // 
      float yPos=pos[1]-yOffset;
      float y=
      (
        xIn<=1-zIn ? y00+d1000*zIn+d0100*xIn :
        y10+d0111-d1011*xIn-zIn*d0111
      );
      tPos[1]=y+yPos;

      Vector3 dPos(VFastTransform,fromWorld,tPos);
      CheckMinMax(minValue,maxValue,dPos);
    }
    // calculate bsphere and bcenter estimation
    Vector3 bCenter = (minValue+maxValue)*0.5;
    // calculate bradius
    float bRadius = minValue.Distance(bCenter);
    animContext.SetMinMax(minValue,maxValue,bCenter,bRadius);
  }
}

Vector3 ForestPlain::AnimatePoint( const ObjectVisualState &vs, int level, int index ) const
{
  if (_skewMatrix==SkewNone) 
  {
    ShapeUsedGeometryLock<> shape(_shape,level);
    // return original position
    return shape->Pos(index);
  }
  else
  {
    return base::AnimatePoint(vs,level,index);
  }
}

bool ForestPlain::IsPointAnimated(int level, int index) const
{
  return _skewMatrix==SkewNone;

}

#pragma optimize("",on) // default optimization
void ForestPlain::Deanimate( int level, bool setEngineStuff )
{
  //base::Deanimate(level);
}

RString LandscapeObjectName(const char *shape);

RoadType::RoadType()
{
}


RoadType::RoadType( const char *name )
{
  _shape=Shapes.New(name,false,true);
  // scan shape for selections to hide
  const Shape *geom = _shape->GeometryLevel();
  if (geom && geom->NFaces()>0)
  {
    // some shapes (bridges) should not be land-following
    // may have destroy animation
    //LogF("Road with geometry %s",name);
    // try to find a config entry
    ConstParamEntryPtr entry = (Pars>>"CfgNonAIVehicles").FindEntry(LandscapeObjectName(name));
    if (entry)
    {
      _destroyAnim = new ObjectDestroyAnim(_shape,(*entry)>>"destruction");
      LogF("Road with destruction anim %s",name);
    }
    _geometry = true;
  }
  else
  {
    if( (_shape->Special()&OnSurface)==0 )
    {
      LogF("Missing OnSurface %s",name);
    }
    _shape->OrSpecial(NoShadow|OnSurface|NoZWrite);
    //_shape->OrSpecial(NoShadow|OnSurface|NoAlphaWrite);
    //_shape->AndSpecial(~NoZWrite);
    // no occlusions on roads 
    _shape->SetCanBeOccluded(false);
    for( int level=0; level<_shape->NLevels(); level++ )
    {
      // TODO: check if modification of shape is legal
      ShapeUsed lock = _shape->Level(level);
      if (lock.IsNull()) continue;
      Shape *shape = lock.InitShape();
      shape->ClipOrAll(ClipLandOn);
    }
    if (_shape->GetAnimationType()==AnimTypeSoftware)
    {
      //LogF("SW Animation detected on road %s",name);
      _shape->SetAnimationType(AnimTypeNone);
    }
    _geometry = false;
  }
}


RoadType::~RoadType()
{
}


DEFINE_FAST_ALLOCATOR(Road)
DEFINE_CASTING(Road)

Road::Road( LODShape *shape, const CreateObjectId &id )
:Object(shape,id)
{
  _roadType=RoadTypes.New(shape->Name());
  SetType(Network);
  if (_roadType->_geometry)
  {
    SetDestructType(DestructBuilding);
  }
  else
  {
    SetDestructType(DestructNo);
  }
}

void Road::DrawDiags()
{
  // draw star on position of 
  //GScene->DrawCollisionStar(Position(),3);
}


/** Cf. LandSegment::MergeAndSnapRoads for the same condition implemented
*/
Object::Visible Road::VisibleStyle() const
{
  /// check if the road is merged into the landscape rendering
  return Landscape::_roadsIntoSegment && !_roadType->_geometry ? ObjInvisible : ObjVisibleFast;
}

ObjectDestroyAnim *Road::GetDestroyAnimation() const
{
  if (!_roadType->_destroyAnim) return NULL;
  return _roadType->_destroyAnim->Validate();
}

/**
Roads cannot be instanced because of terrain-following behavior.
*/
bool Road::CanBeInstanced(int level, float distance2, const DrawParameters &dp) const
{
  // normal roads can be instanced
  return (GetDestructType()==DestructNo);
}

bool Road::OcclusionFire() const
{
  // only roads with non-empty geometry can occlude or can be hit by bullets
  return _roadType->_geometry;
}
bool Road::OcclusionView() const
{
  return _roadType->_geometry;
}

bool Road::FixedHeight() const
{
  return _roadType->_geometry;
}

AnimationStyle Road::IsAnimated( int level ) const
{
  if (GetDestructType()==DestructNo) return NotAnimated;
  return base::IsAnimated(level);
}
int Road::PassOrder( int lod ) const {return 1;}

const float roadArmor = 1200;
const float roadInvArmor = 1/roadArmor;
const float roadLogArmor = log(roadArmor);

float Road::GetArmor() const
{
  return roadArmor;
}
float Road::GetInvArmor() const
{
  return roadInvArmor;
}
float Road::GetLogArmor() const
{
  return roadLogArmor;
}

#if _ARMA3_TERRAIN_DECALS

DEFINE_FAST_ALLOCATOR(TerrainDecal)
DEFINE_CASTING(TerrainDecal)

TerrainDecal::TerrainDecal( LODShape *shape, const CreateObjectId &id )
:Road(shape,id)
{
  SetType(LandDecal);
}

#endif //#if _ARMA3_TERRAIN_DECALS

DEF_RSB(forest);
DEF_RSB(road);
#if _ARMA3_TERRAIN_DECALS
DEF_RSB(land_decal);
#endif
DEF_RSB(streetlamp);
DEF_RSB(house);
DEF_RSB(vehicle);
DEF_RSB(church);
DEF_RSB(tower);
DEF_RSB(treehard);
DEF_RSB(treesoft);
DEF_RSB(bushhard);
DEF_RSB(bushsoft);

/// see WindObjectType
struct WindAnimDefault
{
  static float GetFrequency0ms() {return 0;}
  static float GetFrequency10ms() {return 0;}
  static float GetAmplitude() {return 0;}
  static float GetTilt() {return 0;}
  static bool GetOrientationLast() {return false;}
  /** ln(0.1)/1  - 0.1 visibility for 1 m distance */
  static float ViewDensity() {return -2.302585093f;}
  
};

struct WindAnimParTreeHard: public WindAnimDefault
{
  static float GetFrequency0ms() {return 0.25f;}
  static float GetFrequency10ms() {return 0.25f;}
  static float GetAmplitude() {return 0.002f;}
};

struct WindAnimParTreeSoft: public WindAnimDefault
{
  static float GetFrequency0ms() {return 0.25f;}
  static float GetFrequency10ms() {return 0.35f;}
  static float GetAmplitude() {return 0.003f;}
  static float GetTilt() {return 0.005f;}
};
struct WindAnimParBushHard: public WindAnimDefault
{
  static float GetFrequency0ms() {return 0.35f;}
  static float GetFrequency10ms() {return 0.70f;}
  static float GetAmplitude() {return 0.003f;}
  static bool GetOrientationLast() {return true;}
  static float GetTilt() {return 0.02f;}
  /** ln(0.1)/0.5  - 0.1 visibility for 0.5 m distance */
  static float ViewDensity() {return -4.605170186f;}
};
struct WindAnimParBushSoft: public WindAnimParBushHard
{
  static float GetFrequency0ms() {return 0.5f;}
  static float GetFrequency10ms() {return 1.0f;}
  static float GetAmplitude() {return 0.004f;}
  static float GetTilt() {return 0.1f;}
};

/// specific implementation of wind waving
template <class WindAnimPar>
class ObjectWindAnimPar: public ObjectWindAnim
{
  typedef ObjectWindAnim base;
  
  public:
  ObjectWindAnimPar(LODShape *shape, const CreateObjectId &id)
  :ObjectWindAnim(shape,id)
  {
    // make sure fast allocator of ObjectWindAnim is still valid
    COMPILETIME_COMPARE(sizeof(ObjectWindAnimPar),sizeof(base));
  }
  
  WindObjectType GetWindType() const
  {
    return WindObjectType(
      WindAnimPar::GetFrequency0ms(),
      WindAnimPar::GetFrequency10ms(),
      WindAnimPar::GetAmplitude(),
      WindAnimPar::GetTilt(),
      WindAnimPar::GetOrientationLast()
    );
  }
  /// tree have usually some transparency
  float ViewDensity() const {return WindAnimPar::ViewDensity();}
};


//@{ various wind waving types
typedef ObjectWindAnimPar<WindAnimParTreeHard> ObjectWindAnimTreeHard;
typedef ObjectWindAnimPar<WindAnimParTreeSoft> ObjectWindAnimTreeSoft;
typedef ObjectWindAnimPar<WindAnimParBushHard> ObjectWindAnimBushHard;
typedef ObjectWindAnimPar<WindAnimParBushSoft> ObjectWindAnimBushSoft;
//@}

Object *NewObject( LODShape *shape, const CreateObjectId &id )
{
  const RStringB &className=shape->GetPropertyClass();
  if( className.GetLength()>0 )
  {
    if( className==RSB(forest) )
    {
      /*
      ForestType *type = dynamic_cast<ForestType *>(VehicleTypes.FindShape(shape->Name()));
      if( type )
      {
        type->VehicleAddRef();
        Object *obj=new Forest(type,id);
        type->VehicleRelease();
        return obj;
      }
      */
      //RptF("%s: no forest in config",(const char *)shape->Name());
      return new ForestPlain(shape,id); 
    }
    else if(className==RSB(road)) return new Road(shape,id);

#if _ARMA3_TERRAIN_DECALS
    else if(className==RSB(land_decal)) return new TerrainDecal(shape, id);
#endif
    else if(className==RSB(streetlamp))
    {
      Ref<EntityType> vehType = VehicleTypes.FindShapeAndSimulation(shape->Name(),className);
      if (!vehType) vehType = VehicleTypes.FindShape(shape->Name());
      if (!vehType)
      {
        RptF("%s: %s, config class missing",shape->Name(),(const char *)className);
        return new ObjectPlain(shape,id);
      }
      StreetLampType *type = dynamic_cast<StreetLampType *>(vehType.GetRef());
      if (!type)
      {
        LogF(
          "%s: %s, config class %s not StreetLampType",
          shape->Name(),cc_cast(className),cc_cast(vehType->GetName())
        );
        return new ObjectPlain(shape,id);
      }
      return new StreetLamp(shape,type,id);
    }
    else if
    (
      className==RSB(house) ||
      className==RSB(vehicle) ||
      className==RSB(tower) ||
      className==RSB(church)
    )
    {
      // search vehicle type bank for given shape
      // prefer type with the same simulation
      Ref<EntityType> vType=VehicleTypes.FindShapeAndSimulation(shape->Name(),className);
      if (!vType)
      {
        // if not found, resort to any type with this shape
        vType=VehicleTypes.FindShape(shape->Name());
      }
      if( !vType )
      {
        // it is not in config: ignore it 
        RptF("%s: %s, config class missing",shape->Name(),(const char *)className);
        return new ObjectPlain(shape,id);
      }
      EntityAIType *type=dynamic_cast<EntityAIType *>(vType.GetRef());
      if( !type )
      {
        Fail("Non-ai EntityAIType");
        return new ObjectPlain(shape,id);
      }
      RString sim=type->_simName;
      if( sim.GetLength()<=0 )
      {
        RptF("No simulation: %s",(const char *)type->GetName());
        return new ObjectPlain(shape,id);
      }
      if (type->IsAbstract())
      {
        RptF("Cannot create object with abstract type %s (scope = private?)",cc_cast(type->GetName()));
        return new ObjectPlain(shape,id);
      }
      type->VehicleAddRef();
      Object *building=NULL;
      //else if( !strcmpi(sim,"flag") ) building=new Flag(type,id);
      if( !strcmpi(sim,"house") ) building=new Building(type,id);
      else if( !strcmpi(sim,"church") ) building=new Church(type,id);
      else if( !strcmpi(sim,"fountain") ) building=new Fountain(type,id);
      else if( !strcmpi(sim,"vasi") ) building=new VASILights(type,id);
      else if( !strcmpi(sim,"breakablehouseanimated") ) building=new Building(type,id);
      else if (!stricmp(sim,"flagcarrier") )
        building = new ObjectPlain(shape,id);
      else if ( !stricmp(sim,"collection") ) 
        building = new ObjectPlain(shape,id);
      else
      {
        RptF("Bad simulation %s, type %s (class=%s), %s", (const char *)sim, cc_cast(type->GetName()), cc_cast(className), cc_cast(shape->GetName()));
        building = new ObjectPlain(shape,id);
      }
      type->VehicleRelease();
      if (building)
      {
        if (GLandscape)
        {
          int index = GLandscape->FindBuildingIndex(building->GetObjectId());
          if (index >= 0)
            building->SetBuildingIndex(index);
        }
        return building;
      }
    }
    else if (className==RSB(treehard))
      return new ObjectWindAnimTreeHard(shape,id);
    else if (className==RSB(treesoft))
      return new ObjectWindAnimTreeSoft(shape,id);
    else if (className==RSB(bushhard))
      return new ObjectWindAnimBushHard(shape,id);
    else if (className==RSB(bushsoft))
      return new ObjectWindAnimBushSoft(shape,id);
    else
      Log("Unknown object class '%s' (%s)",cc_cast(className),cc_cast(shape->GetName()));
  }
  return new ObjectPlain(shape,id);
}

RoadTypeBank RoadTypes;


