#ifdef _MSC_VER
#pragma once
#endif

#ifndef _OBJECTCLASSES_HPP
#define _OBJECTCLASSES_HPP

#include <El/ParamFile/paramFile.hpp>

#include "object.hpp"
#include "transport.hpp"
#include "lights.hpp"
#include "house.hpp"
#include "wounds.hpp"

class StreetLampType: public EntityType
{
  typedef EntityType base;
  friend class StreetLamp;

  protected:
  int _bulbHit;
  
  float _radius;
  Color _colorDiffuse;
  Color _colorAmbient;

  public:
  StreetLampType( ParamEntryPar param );
  ~StreetLampType();

  virtual void Load(ParamEntryPar par, const char *shape);
  virtual void InitShape(); // after shape is loaded
  virtual void DeinitShape(); // before shape is unloaded
  bool AbstractOnly() const {return false;}

  virtual Object* CreateObject(bool unused) const;
};

class StreetLamp: public Entity
{
  typedef Entity base;
public:
  enum LightState
  {
    LSOff,
    LSOn,
    LSAuto
  };
protected:
  LightState _lightState;
  bool _pilotLight; // switch the light on/off
  Ref<LightReflector> _light;
  Vector3 _lightPos;
  
public:
  StreetLamp( LODShape *shape, const StreetLampType *type, const CreateObjectId &id );

  __forceinline const StreetLampType *Type() const
  {
    return static_cast<const StreetLampType *>(_type.GetRef());
  }

  LightState GetLightState() const {return _lightState;}
  void SwitchLight(LightState state);

  void Init( Matrix4Par pos, bool init );
  void SimulateSwitch();
  void ResetStatus();
  void OnTimeSkipped();
  void CreateLight(Matrix4Par pos);
  bool SimulationReady(SimulationImportance prec) const;
  void Simulate( float deltaT, SimulationImportance prec );

  void HitBy( EntityAI *killer, float howMuch, RString ammo, bool wasDestroyed);

  LSError Serialize(ParamArchive &ar);

  USE_CASTING(base)
};

class VASILightsType: public EntityAIType
{
  typedef EntityAIType base;
  friend class VASILights;

  protected:
  //HitPoint _bulbHit;
  
  float _slope;
	bool _enabled;
  //@{ changing color is done via hinding sections
  AnimationSection _white,_red,_off;
  //@}

  public:
  VASILightsType( ParamEntryPar param );
  ~VASILightsType();

  virtual float GetFeatureSize(Vector3Par pos) const {return 50;} // VASI lights need to be rendered very far
  
  virtual void Load(ParamEntryPar par, const char *shape);
  virtual void InitShape(); // after shape is loaded
  virtual void DeinitShape(); // before shape is unloaded
	virtual void InitShapeLevel(int level);
	virtual void DeinitShapeLevel(int level);
  bool AbstractOnly() const {return false;}

  virtual Object* CreateObject(bool unused) const;
};

class VASILights: public EntityAI
{
  typedef EntityAI base;
protected:
  //LightState _lightState;
  //bool _pilotLight; // switch the light on/off
  //Ref<LightPointVisible> _light;
  //Vector3 _lightPos;
  
  // Object to represent the point light. It will be added to pass2
  Ref<PointLight> _pointLight;
  
public:
  VASILights(const EntityAIType *type, const CreateObjectId &id);

  __forceinline const VASILightsType *Type() const
  {
    return static_cast<const VASILightsType *>(_type.GetRef());
  }

  bool QIsManual() const {return false;}
  bool QCanIGetIn( Person *who = NULL ) const {return false;}

  //@{ VASI is animated using material - no instancing
  AnimationStyle IsAnimated(int level) const {return AnimatedTextures;}
  virtual bool CanBeInstanced(int level, float distance2, const DrawParameters &dp) const {return false;}
  void Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2);
  void Deanimate(int level, bool setEngineStuff);
  int PassNum( int lod );
  virtual bool NeedsPrepareProxiesForDrawing() const {return true;}
  virtual void PrepareProxiesForDrawing(
    Object *rootObj, int rootLevel, const AnimationContext &animContext,
    const PositionRender &transform,int level, ClipFlags clipFlags, SortObject *so);
  //@}
  void Init(Matrix4Par pos, bool init);
  void ResetStatus();
  bool SimulationReady(SimulationImportance prec) const;
  void Simulate( float deltaT, SimulationImportance prec );

  void HitBy( EntityAI *killer, float howMuch, RString ammo, bool wasDestroyed );

  virtual float MinPixelArea(float minPixelAuto) const;
  virtual float EstimateArea() const;
  virtual float DensityRatio() const;
  virtual float DrawingAreaRatio() const;

  LSError Serialize(ParamArchive &ar);
};

class ObjectDestroyAnim;

/// road object shared information
class RoadType: public RefCount
{
  friend class Road;
  Ref<LODShape> _shape;
  RefR<ObjectDestroyAnim> _destroyAnim;
  /// some road have a significant geometry, but most do not
  bool _geometry;

  public:
  const char *GetName() const {return _shape->Name();}

  RoadType();
  RoadType( const char *name );
  ~RoadType();

  private:
  RoadType(const RoadType &src);
  void operator =(const RoadType &src);
};

//TypeIsMovable(RoadType);

class RoadTypeBank: public BankArray<RoadType>
{ 
};

extern RoadTypeBank RoadTypes;

#include <Es/Memory/normalNew.hpp>

class Road: public Object
{
  typedef Object base;

  InitPtr<RoadType> _roadType;

  public:
  Road( LODShape *shape, const CreateObjectId &id );

  ObjectDestroyAnim *GetDestroyAnimation() const;
  
  AnimationStyle IsAnimated( int level ) const; // appearance changed with Animate
  virtual bool CanBeInstanced(int level, float distance2, const DrawParameters &dp) const; // appearance changed with Animate
  bool OcclusionFire() const;
  bool OcclusionView() const;
  int PassOrder( int lod ) const;
  virtual bool FixedHeight() const;
  const RoadType *GetRoadType() const {return _roadType;};

  float GetArmor() const;
  float GetInvArmor() const;
  float GetLogArmor() const;

  void DrawDiags();
  
  /// some roads use other rendering method
  virtual Visible VisibleStyle() const;

  USE_FAST_ALLOCATOR
  USE_CASTING(base)
};

#if _ARMA3_TERRAIN_DECALS

class TerrainDecal: public Road
{
  typedef Road base;

public:
  TerrainDecal( LODShape *shape, const CreateObjectId &id );

  USE_FAST_ALLOCATOR
  USE_CASTING(base)
};

#endif //#if _ARMA3_TERRAIN_DECALS

#define FOREST_PROXY_ENABLE 0

class ForestPlain: public Object
{
  typedef Object base;
  enum SkewMatrix {SkewNone,SkewT1,SkewT2,SkewPos};
  SkewMatrix _skewMatrix;
  
  public:
  ForestPlain( LODShape *type, const CreateObjectId &id );

  virtual bool InitSkewReplace(Landscape *land, Matrix4 &trans); // call to prepare skew matrix
  virtual void InitSkew(Landscape *land);
  virtual void DeSkew(const Landscape *land, Matrix4& trans, float& relHeight);

  AnimationStyle IsAnimated( int level ) const; // appearance changed with Animate
  void Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2);
  void Deanimate( int level, bool setEngineStuff );

  Vector3 AnimatePoint(const ObjectVisualState &vs, int level, int index) const;
  virtual bool IsPointAnimated(int level, int index) const;
  float ViewDensity() const;
  virtual bool FixedHeight() const {return true;}

  #if !FOREST_PROXY_ENABLE
  bool DrawProxiesNeeded(int level) const {return false; }
  virtual int GetProxyComplexity(int level, const Matrix4 &pos, float dist2) const {return 0;}

  // proxy access
  virtual int GetProxyCount(int level) const {return 0;}
  #endif


  USE_CASTING(base)

  USE_FAST_ALLOCATOR
};


#include <Es/Memory/debugNew.hpp>

#endif
