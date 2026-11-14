/*!
\file
Scene interface

Scene defines lights and camera positions, performs LOD and rendering management.
*/

#ifdef _MSC_VER
#pragma once
#endif

#ifndef _SCENE_HPP
#define _SCENE_HPP


#include "types.hpp"
#include <El/Math/math3d.hpp>
#include <Es/Containers/smallArray.hpp>
//#include "object.hpp"
//#include "engine.hpp"
#include "lights.hpp"
#include "camera.hpp"
#if _ARMA3_NEW_LIGHTING_CFG
#include "landLighting.hpp"
#endif

#define HORIZONT_Z Glob.config.horizontZ
#define OBJECT_Z Glob.config.objectsZ

//! dayz lensflares timeout
#define DAYZ_LENSFLARES_TIMEOUT	0.25f


//! Distance limit of the object designed for depth buffer rendering
// See label DODL
const float DepthObjectsDistanceLimit = 50.0f;

namespace enf
{
	class ShapeCollection;
}

#define LEN_FOG_TABLE 256
class FogFunction
{
  private:
  byte _fog[LEN_FOG_TABLE];
  float _start2,_end2;
  float _divisor;

  public:
  FogFunction();
  void Set(
    float start, float end, float expFogCoef,
    float (*function)(float distRel, float start, float end, float expFogCoef)
  );
  int operator () ( float distSquare ) const; // avoid partial stall
};

/// distance discretization for preloading / LOD handling
/**
Radius of i-th sphere is: r(i) = _minDist*sqrt(_kCoef)^i).
Sphere radius / area is a geometric sequence.
This corresponds to LOD / texture mipmaps being geometric sequence.
*/

template <int maxSlots>
class DiscreteSlots
{
  float _kCoef;
  float _minDist;
  float _logMinDist2;
  float _invLogK;
  float _logSqrtK;

  /// table of minValues to avoid repeated exp calculation
  AutoArray<float, MemAllocDataStack<float,maxSlots> > _minValues;
  
  public:
  DiscreteSlots(float kCoef, float minDist, int nSlots)
  {
    _kCoef = kCoef;
    _minDist = minDist;
    float logKCoef = log(_kCoef);
    _invLogK = 1/logKCoef;
    _logMinDist2 = log(_minDist)*2;
    _logSqrtK = logKCoef*0.5f;
    
    _minValues.Resize(nSlots);
    for (int i=0; i<nSlots; i++)
    {
      // TODO: incremental calculation of
      _minValues[i] = Square(_minDist*exp(i*_logSqrtK));
    }
  }

  /// get a minimum distance for a given sphere
  float MinValue2(int i) const
  {
    return _minValues[i];
  }

  /// get slot index from squared distance
  int SlotFromValue2(float dist2) const
  {
    // convert 0 to minus infinity
    const float limitDist2 = 1e-20f;
    const float logLimitDist2 = -46.05170186f;
    if (dist2<=limitDist2)
    {
      return toInt((logLimitDist2-_logMinDist2)*_invLogK);
    }
    float slotCont = (log(dist2)-_logMinDist2)*_invLogK;
    return toInt(slotCont);
  }
};

class ObjectListFull;

#include <Es/Memory/normalNew.hpp>

class SortObject: public RefCount, private NoCopy
{
  /// object must not be destroyed until it is drawn
  Ref<Object> _object; 
  /// parent object need to be known during drawing of some proxies
  Ref<Object> _parentObject; 
  /// root of the hierarchy of proxies, used to decide about passNum (all proxies need to be drawn in the same pass)
  Ref<Object> _rootObject; 
  
public:
  //@{ multiple instances handled at once
  InitPtr<const ObjectListFull> _list;
  int _listBeg,_listEnd;
  //@}
  
  // simple pointer is enough - Ref<> to object guarantees shape exists
  LODShape *shape; // randomized shape
  PositionRender position;

  struct MatrixCacheItem: public AutoArray<Matrix4>
  {
    /// which frame were the values last updated?
    InitVal<int,0> _frameId;
    // TODO: cleanup old values to reclaim memory used by them?
    ClassIsMovableZeroed(MatrixCacheItem)
  };
  /// one object may have matrices computed for multiple LODs
  mutable AutoArray<MatrixCacheItem>  _matrixCache;
  
  /// number of subobjects we are currently holding as a root
  mutable int _nItems;
  /// index of this object in the rootObject - used for caches of proxy animations
  mutable int _rootIndex;
  /// we may contain some animated proxies as well
  /**
  As proxies may have no SortObject of their own, they may need to use its parent object as a storage
  */
  mutable AutoArray< AutoArray<MatrixCacheItem> > _matrixCacheProxy;
  
  
  signed char drawLOD,shadowLOD,shadowVolLOD,rootLOD;
  /// Source LOD object - used to blend from (both the srcLOD and the dstLOD can have the same value which means blending is not in progress)
  signed char srcLOD;
  /// Destination LOD - used to blend to
  signed char dstLOD;
  /// for groups (_list) - provide a "dither pattern" allowing disappearing of some objects only
  unsigned char _disappearDither;
  
  // test objects index (order in ObjectList) against the disappear pattern
  bool IsVisible(int index) const
  {
    Assert(index>=_listBeg && index<_listEnd);
    int bit = index&7;
    return (_disappearDither>>bit)&1;
  }
  
  /// avoid the same object being prepared twice (features)
  unsigned char _prepareId;
  
  //! Flag to determine the sprite size is to be limited
  bool limitSpriteSize;
  //! Bit array to determine what CSM layers this object will be used in
  unsigned char csmLayerReached;

  signed char passNum;
  signed char forceDrawLOD; // forced draw LOD
  
  /// or clip flags only - andClip would be always 0 
  unsigned char orClip;
  
  /// sometimes we want to force rendering even if data are not ready yet
  bool _someDataNeeded;

  /// how much is the object covered by other objects
  /** can affect LOD selection */
  float _visibleRatio;
  //! Blending factor
  /**
  Used for LOD blending and LOD memory
  negative: no LOD blending active (use drawLOD)
  positive: how much of dstLOD should be blending with srcLOD
  For objects with no blending srcLOD is used when blendingFactor<0.5
  */
  float blendFactor;
  /// when not used in the last frame, we may safely delete it
  int _usedInFrame;

  int GetSBLevel() const {return drawLOD!=LOD_INVISIBLE ? drawLOD : shadowLOD;}

  //Vector3 bCenter; // world space bounding sphere center
  float radius; // bounding sphere radius

  float distance2;
  float coveredArea; // covered area (in pixels)
  // z-coordinate may be needed for z-order sorting
  float zCoord;

  SortObject();
  ~SortObject();

  void UpdateBlendFactor();
  
  void SetObject(Object *obj);
  __forceinline Object *GetObject() const {return _object;}

  void SetParentObject(Object *obj);
  __forceinline Object *GetParentObject() const {return _parentObject;}

  void SetRootObject(Object *obj);
  __forceinline Object *GetRootObject() const {return _rootObject;}

  //! Method compares this object with the specified one and tells if they can be instanced together
  template <class CompareLods>
  bool CanBeInstancedTogether(const SortObject &so, const CompareLods &compareLods, const DrawParameters &dp) const;
  friend class Scene; // we need the Scene to access the allocator
  
  enum BlendAlphaOrigin
  {
    BAO_None, // No blending
    BAO_Src,  // Src is being drawn, use (1 - blendFactor)
    BAO_Dst,  // Dst is being drawn, use blendFactor
  };

  
  float GetBlendingCoef(BlendAlphaOrigin bao, bool canBlend) const;

  /// blend direction - when true, the blend is going dst to src
  bool DirectionReversed() const
  {
    // when desired is closer to src than dst, the blending direction is reversed
    return abs(drawLOD-srcLOD)<abs(drawLOD-dstLOD);
  }
  USE_FAST_ALLOCATOR
};

typedef Array< Ref<SortObject> > SortObjectListArg;

template <int count>
class SortObjectList:
  public RefArray<SortObject, MemAllocDataStack<Ref<SortObject>,count> >
{
};

class RememberSplit: public RefCount, public LinkBidirWithFrameStore
{
  friend class ShadowCache;

  private:
  Ref<Shape> _shadow;
  OLinkOL(Object) _object;
  Matrix4 _objectPos;
  int _level; // which LOD of object is used
  
  //size_t _memoryUsed;
  
  public:
  RememberSplit();
  ~RememberSplit();
  // init shadow or split to fit on surface
  void InitSplit(Object *object, int level, int special);
  bool IsEqualObject(Object *object) const {return _object.IsEqual(object);}
  Matrix4Val ObjectPos() const {return _objectPos;}
  size_t GetMemoryControlled() const;
  bool IsLoaded() const;
  void UnloadGeometry();

  USE_FAST_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>

#include "El/FreeOnDemand/memFreeReq.hpp"

#if _VBS3_LAND_SHADOW
class LandscapeShadowMapCache;
#else
class LandscapeShadowMap;
#endif

class EngineProperties;

/// cached shadows

/** 
- we want to avoid calulation shadows again and again.
*/

class ShadowCache: public MemoryFreeOnDemandHelper
{
  private:
  //
  FramedMemoryControlledList<
    RememberSplit,FramedMemoryControlledListTraitsRef<RememberSplit>
  > _data;

  public:
  ShadowCache();
  ~ShadowCache();

  Ref<Shape> Split(Object *object, Vector3Par lightDir, int level, int special);
  void ShadowChanged(Object *obj);
  void Clear();
#if _VBS3_CRATERS_DEFORM_TERRAIN
  //! Remove from cache all items in the specified vicinity (2D)
  void Clear2DVicinity(Vector3Par xzPosition, float xzRadius);
#endif
  void CleanUp();

  bool CheckIntegrity(RememberSplit *sc) const;
  bool CheckIntegrity(Object *obj) const;
  bool CheckIntegrity(bool detailed=false) const;

  // implement MemoryFreeOnDemandHelper functions
  virtual size_t FreeOneItem();
  virtual float Priority() const;
  virtual size_t MemoryControlled() const;
  virtual size_t MemoryControlledRecent() const;
  virtual void MemoryControlledFrame();
  RString GetDebugName() const;
};

enum PreloadedShape
{
  //ATGMExplosion,LAWExplosion,ShellExplosion,
  //Shell,ATMissile,HeliMissile,HeliRocket,
  CobraLight,SphereLight,HalfLight, Marker,
  CraterShell,CraterBullet,
  SlopBlood,
  CloudletBasic,CloudletFire,CloudletFireD,CloudletWater,CloudletMissile,
//  Cloud1,Cloud2,Cloud3,Cloud4,
//  CinemaBorder,

  FootStepL,FootStepR,

  ForceArrowModel,SphereModel,HalfSpaceModel,RectangleModel,PaperCarModel,
  BulletLine, RayModel, GunLightModel,GizmoModel,

//  SkySphere, HorizontObject,

//  FxExploGround1, FxExploGround2,
//  FxExploArmor1, FxExploArmor2, FxExploArmor3, FxExploArmor4,
//  FxCartridge,
 
  MaxPreloadedShape
};

#ifndef DECL_ENUM_PRELOADED_TEXTURE
#define DECL_ENUM_PRELOADED_TEXTURE
DECL_ENUM(PreloadedTexture)
#endif
#ifndef DECL_ENUM_PRELOADED_MATERIAL
#define DECL_ENUM_PRELOADED_MATERIAL
DECL_ENUM(PreloadedMaterial)
#endif

typedef Ref<Light> ActiveLightPointer;

class LightList: public FindArray< ActiveLightPointer,MemAllocLocal<ActiveLightPointer,32> >
{
  public:
  typedef ConstructTraits<ActiveLightPointer> CTraits;
  typedef FindArray<ActiveLightPointer,MemAllocSS> base;

  LightList();
  LightList( const LightList &src );
  
  bool ContainsIdenticalLights(const LightList &src) const;
  int Compare(const LightList &src) const;
};

/// default textures used on various places in the game (often directly by the engine)
class PreloadedTextures
{
  RefArray<Texture> _data;

  public:
  PreloadedTextures();
  ~PreloadedTextures();

  void Preload( bool all );
  void Clear();

  Texture *New( RStringB name ); // make texture permanent
  Texture *New( PreloadedTexture id ); // predefined texture ids
};

//@{
/// interface to the material preloading
TexMaterial *GlobPreloadMaterial(PreloadedMaterial id);

void GlobPreloadedTexturesPreload(bool all);
void GlobPreloadedTexturesClear();

//@}

extern PreloadedTextures GPreloadedTextures;

#define LIGHTGRID_SIZE 100.0f

#include <Es/Memory/normalNew.hpp>

class GridItemLightList : public LightList, public RefCount
{
  USE_FAST_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>

#include <El/Rectangle/rectangle.hpp>


struct FrustumScanLine
{
	int _min, _max;

	FrustumScanLine(int xmin, int xmax): _min(xmin), _max(xmax) {}
	FrustumScanLine(): _min(0), _max(0) {}
};

TypeIsSimpleZeroed(FrustumScanLine);

struct FlexibleLoop2
{
	AutoArray< FrustumScanLine, MemAllocLocal<FrustumScanLine,256> > _scanLines;
	int _beg, _end;
	int _majorOffset;
	int _majorInc;
	bool _zMajor;

	inline FlexibleLoop2(): _beg(0), _end(0), _majorOffset(0), _majorInc(0), _zMajor(false)
	{
	}

	inline int GetMajorBegin() const
	{
		return _beg;
	}
	inline int GetMajorEnd() const
	{
		return _end;
	}
	inline int GetMajorIncrement() const
	{
		return _majorInc;
	}
	inline int GetMinorIncrement() const
	{
		return 1;
	}
	inline int GetMinorBegin(int major) const
	{
		return _scanLines[major - _majorOffset]._min;
	}
	inline int GetMinorEnd(int major) const
	{
		return _scanLines[major - _majorOffset]._max;
	}
	inline int GetX(int minor, int major) const
	{
		return _zMajor ? minor: major;
	}
	inline int GetZ(int minor, int major) const
	{
		return _zMajor ? major: minor;
	}
};


//! Scene rendering
/*!
This class performs scene graph management.
This includes lod and light management.
*/
class Scene
{
private:

  Color _constantColor;
  Color _skyColor;
  Color _skyTopColor;

  //Engine *_engine;
  //! Sky texture for cases where no weather is used
  Ref<Texture> _skyTexture;
  //! Sky reflection texture for cases where no weather is used
  Ref<Texture> _skyTextureR;

  Ref<LODShape> _preloaded[MaxPreloadedShape];

#if _VBS3_LODTRANSITIONCONTROL
  //! Speed of LOD transition (used f.i. by trees)
  float _lodTransitionSpeed;
#endif

  Camera *_camera;
  LightSun *_mainLight;

	#if _ARMA3_NEW_LIGHTING_CFG
	 /// current lighting for the scene
	 LandLighting::LightingResult _currentLighting;
	#endif

	AverageTimeDeltaValue _smoothLensSunTime;							///< smoother time value
	AverageTimeDeltaValue _smoothLensFlaresTime;					///< smoother time value

  //! List of real lights in the scene (used for light volume / area / point rendering)
  FindArray< Link<Light> > _realLights;
  //! List of aggregated lights in the scene (used to light the scene)
  FindArray< Link<Light> > _aggLights;

  //! Most important lights stored in a grid
  /** Note that each grid item corresponds the top-left corner of the represented area (not the center) */
  RefArray<GridItemLightList> _gridLights;
  /// permanent list of no lights
  LightList _noLights;
  //! Position and dimension of the light grid
  GridRectangle _gridLightsBoundingRect;

  //! List of active real lights (lights in the player proximity)
  LightList _aRealLights;
  //! List of active aggregated lights (lights in the player proximity)
  LightList _aAggLights;
  
  Ref<Object> _collisionStar;

  // fog functions for different types of objects
  FogFunction _fog,_tacticalFog;
  float _tacticalVisibility; // AI sensors
  float _rainRange; // display boundaries main control
  float _fogMaxRange,_fogMinRange; // display boundaries
  float _hazeDefaultDistance; // display boundaries

  mutable float _drawDensity;

  //! Scene density set in AdjustComplexity function
  float _density;
  
  /// used to apply LOD transitions in this frame
  float _thisFrameDeltaT;
  /// scene complexity target
  float _complexityTarget;
  /// min. pixel size of the object to draw - min. distance
  float _minPixelSizeMinDist;
  /// min. pixel size of the object to draw - mid. distance
  /** mid. distance is the distance at which rifle engagement is expected to be done - arund 200 m */
  float _minPixelSizeMidDist;
  /// min. pixel size of the object to draw - max. distance
  float _minPixelSizeMaxDist;
  /// pixel shader quality
  int _shadingQuality;

  float _shadowDistanceZ;

	//@{To avoid LHS and recomputing
	float _engineWidth;
	float _engineHeight;
	float _engineArea;
	//@}

  enum {NStoreComplexities = 4};

  mutable int _lastComplexity[NStoreComplexities]; // complexity history

  //! Number to characterize level of used material LODs. The bigger the rougher, 1 is the finnest one
  //float _materialComplexity;

public:
  struct DrawContext
  {
    static const int MaxSceneObjects = 16384;
    /// pass 1O (opaque) objects - near (before landscape)
    struct DrawPass1ON
    {
      SortObjectList<MaxSceneObjects*1/16> _original;
      SortObjectList<MaxSceneObjects*1/16> _blending;
    } _drawPass1ON;
    /// pass 1O (opaque) objects - far or slightly alpha (after landscape)
    struct DrawPass1OF
    {
      SortObjectList<MaxSceneObjects*3/16> _original;
      SortObjectList<MaxSceneObjects*3/16> _blending;
    } _drawPass1OF;
    /// pass 1A (almost opaque) objects - near
    SortObjectList<MaxSceneObjects*1/8> _drawPass1AN;
    /// pass 1A (almost opaque) objects - far
    SortObjectList<MaxSceneObjects*1/8> _drawPass1AF;
    /// on surface objects (roads, tracks, craters)
    SortObjectList<MaxSceneObjects*1/16> _drawOnSurfaces;
    /// pass 2 (alpha objects)
    SortObjectList<MaxSceneObjects*1/8> _drawPass2;
    /// pass 3 (cockpit objects)
    SortObjectList<32> _drawPass3;
    /// pass 4 (cockpit objects)
    SortObjectList<8> _drawPass4;
    /// shadow buffer pass objects
    SortObjectList<MaxSceneObjects*1/16> _drawShadowProj;
    /// volume shadow pass objects
    SortObjectList<MaxSceneObjects*1/8> _drawShadowVol;

		#ifdef TEST_RAIN_MAP
		   SortObjectList<MaxSceneObjects/16> _drawRainMap;
		#endif
    
    //@{ give engine a space to remember predicated scopes for reuse
    Engine::PredicatedCBScope _oPasO;
    Engine::PredicatedCBScope _oPasA;
    Engine::PredicatedCBScope _oPas3;
    Engine::PredicatedCBScope _oPas4;
    Engine::PredicatedCBScope _lDGnd;
    Engine::PredicatedCBScope _lDGCl;
    //@} 
  };
  
  // IR ray, rays are drawn deffered
  class IRLasers
  {
    struct IRRaySource
    {
      const Person *_person;
      Vector3 _dir;
      Vector3 _pos;
      Vector3 _endPos;
      bool _isBehind;

      ClassIsSimple(IRRaySource)
    };

    struct IRRaySort
    {
      float _distToPlayer;
      IRRaySource *_ir;   

      static int Compare(const IRRaySort *w0, const IRRaySort *w1)
      {
        float diff = w0->_distToPlayer - w1->_distToPlayer;
        if(diff > 0) return -1;
        if(diff < 0) return +1;
        return 0;
      }

      ClassIsSimple(IRRaySort)
    };

    AutoArray<IRRaySource> _irLasers;
    
  public:
    ~IRLasers() { Clear(); }
    // draw ray
    void Draw();
    // add IR into list
    void Register(const Person *person, bool reg, Vector3Val pos, Vector3Val dir);
    // remove all stuff
    void Clear() { _irLasers.Clear(); }
  };

  private:
  /// list of objects used in scene graph
  RefArray<SortObject> _drawObjects;
  
#if _HELISIM_RENDER_TO_TEXTURE
public:
  // set current _drawObjects
  bool SetDrawObjectsIndex(int i);

  int GetDrawObjectIndex() const { return _drawObjectsId; }
  // releases from all _drawObjectsStore
  void ReleaseDrawObject(Object *o);

private:
  int _drawObjectsId;
  RefArray<SortObject> _drawObjectsStore[Object::NumDrawObjects];
#endif

  /// each Prepare call is given Id - used to avoid the same object being prepared twice (features)
  unsigned char _prepareId;

  /// list of temporary objects storing state of proxies
  RefArray<Object> _proxyObjects;

  /// list of diagnostics objects to be rendered
  RefArray<Object> _arrows;

  struct DiagText
  {
    RString _text;
    Color _color;
    Vector3 _pos;
    float _size;
    int _priority;

    ClassIsMovableZeroed(DiagText)
  };

  /// list of diagnostic texts to be shown
  AutoArray<DiagText> _diagText;

  mutable ShadowCache _shadowCache;
  SRef<Landscape> _landscape; // only pointer to landscape

  /// landscape shadow casting map
#if _VBS3_LAND_SHADOW
  SRef<LandscapeShadowMapCache> _shadowMapCache;
#else
  SRef<LandscapeShadowMap> _shadowMap;
#endif

  SRef<EngineProperties> _engineProps;
  /// query used to track sun visibility - used for lens flare if available
  Ref<IVisibilityQuery> _sunQuery;
  
  /// shadow quality
  int _shadowQuality;
#if _HELISIM_SHADOW_ZDISTANCE
  float _shadowZDistance;
#endif
  //@{ user preferred values
  float _preferredTerrainGrid;
  float _preferredViewDistance;
  //@}
  //@{ default values, used when no profile is selected and in multiplayer
  float _defaultViewDistance;
  float _defaultTerrainGrid;
#if _HELISIM_VIEW_DIST
  float _preferredObjectViewDistance;
  float _defaultObjectViewDistance;
#endif
#if _HELISIM_AMBIENT_TRAFFIC
  float _preferredTrafficViewDistance;
  float _defaultTrafficViewDistance;
#endif
  //@}

	enf::ShapeCollection* _enfShapeCollection;

  IRLasers _irLasers;
public:

  Scene();
  void Init(Engine *engine, Landscape *landscape, const char *skyTexture, const char *skyTextureR);
  void ResetFog();
  void CleanUp();
  void CleanUpDrawObjects();
  ~Scene();

	enf::ShapeCollection* GetEnfShapeCollection()
	{
		return _enfShapeCollection;
	}

	void RenderDebugShapes();
	int GetEnfDebugShapesCount() const;
	void ClearEnfShapeCollection(bool all);

  bool GetObjectShadows() const {return _shadowQuality>0;}
  float GetComplexityTarget() const {return _complexityTarget;}
  void SetComplexityTarget(float value);
  
  int GetShadowQuality() const {return _shadowQuality;}
  void SetShadowQuality(int value);
  
  int GetShadingQuality() const {return _shadingQuality;}
  void SetShadingQuality(float val);

	float GetEngineArea() const {return _engineArea;}

  /// get minimal terrain grid based on view distance and benchmark value
  float GetMinimalTerrainGrid() const;
  float GetDefaultTerrainGrid() const {return _defaultTerrainGrid;}
  float GetPreferredTerrainGrid() const {return _preferredTerrainGrid;}
  void SetPreferredTerrainGrid(float x);
  
  float GetDefaultViewDistance() const {return _defaultViewDistance;}
  float GetPreferredViewDistance() const {return _preferredViewDistance;}
  void SetPreferredViewDistance(float x);

#if _HELISIM_VIEW_DIST
  float GetDefaultObjectViewDistance() const {return _defaultObjectViewDistance;}
  float GetPreferredObjectViewDistance() const;
  void SetPreferredObjectViewDistance(float x, bool update = true);
#endif

#if _HELISIM_AMBIENT_TRAFFIC
  float GetDefaultTrafficViewDistance() const {return _defaultTrafficViewDistance;}
  float GetPreferredTrafficViewDistance() const;
  void SetPreferredTrafficViewDistance(float x);
#endif

#if _HELISIM_SHADOW_ZDISTANCE
  void SetShadowZDistance(float zDistance);
  float GetShadowZDistance() const { return _shadowZDistance; }
#endif

  /// diagnostics - current visual quality indication
  float GetDrawDensity() const {return _drawDensity;}

  //! Function to return the scene density
  float GetDensity() const {return _density;}
  
  float GetLodTransitionDelta() const
  {
#if _VBS3_LODTRANSITIONCONTROL
    return _lodTransitionSpeed*_thisFrameDeltaT;
#else
    return _thisFrameDeltaT;
#endif
  }
  
  void SetRenderingDeltaT(float deltaT){_thisFrameDeltaT=deltaT;}
  
  Camera *GetCamera() {return _camera;}
  const Camera *GetCamera() const {return _camera;}
  void SetCamera( const Camera &camera );

  void SetConstantColor( ColorVal color ) {_constantColor=color;}
  ColorVal GetConstantColor() const {return _constantColor;}

  const Matrix4 &ScaledInvTransform() const;
  const Matrix3 &CamNormalTrans() const;
  const Matrix4 &CamInvTrans() const;

  Texture *SkyTexture() const {return _skyTexture;}
  
  void ResetLights();
  
  void AddLightReal(Light *light);
  void AddLightAgg(Light *light);
  void AddLight(Light *light)
  {
    AddLightReal(light);
    AddLightAgg(light);
  }

  //Light *GetLight( int i ) const {return _lights[i];}
  //int NLights() const {return _lights.Size();}
  int NLightsAgg() const {return _aggLights.Size();}
  Light *GetLightAgg(int i) const {return _aggLights[i];}

  //! Get index into grid of lights associated with the specified location
  int GetLightGridIndex(float x, float z) const;
  //! Get list of lights for the specified location
  const LightList &GetLightGridItem(float x, float z) const;
  //! Get list of lights for the specified location
  const LightList &GetLightGridItem(int idx) const {return *_gridLights[idx];}
  //! Selects the most important lights and moves them into a 2D grid
  void SelectLightsIntoGrid(bool r2t = false);

  //@{ _shadowMap access

  float GetLandShadowDepthSmooth(float x, float z) const;
  float GetLandShadowSmooth(Vector3Par pos) const;

#if _VBS3_LAND_SHADOW
  void GetSegmentLandShadow(GridRectangle& rectSD, int lod, VertexCustomInfo* custom);
  void CalculateLandShadows();
  void CalculateShadowBigRect();
  void ResetLandShadows(bool invalidateSegments = false);
  void GetLandShadowValuesShape(GridRectangle& rectSD, const AutoArray<Vector3>& pos, AutoArray<VertexCustomInfo>& custom);
#else
  float GetLandShadow(int x, int z) const;
  float GetLandShadowDepth(int x, int z) const;
  void CalculateLandShadows(const GridRectangle &bigRect, bool init);
  void ResetLandShadows();
#endif
  //@}

	void ResetLensFlaresTime();

  void SelectActiveRealLights(Object *dimmed);
  void SelectActiveAggLights(Object *dimmed);
  void SelectActiveLights(Object *dimmed)
  {
    SelectActiveRealLights(dimmed);
    SelectActiveAggLights(dimmed);
  }
  void SetActiveRealLights(const LightList &lights);
  void SetActiveAggLights(const LightList &lights);
  void SetActiveLights(const LightList &lights)
  {
    SetActiveRealLights(lights);
    SetActiveAggLights(lights);
  }
  //const LightList &ActiveRealLights() const {return _aRealLights;} // Not necessary
  const LightList &ActiveAggLights() const {return _aAggLights;}

  //! Select light affecting given position
  const LightList &SelectAggLights(
    Vector3Par pos, float radius, LightList &work
  ); // may return work or something else

  //! Select light affecting given object
  const LightList &SelectAggLights(
    Vector3Par objPos, const Object *object, int level, LightList &work
  );
  
  LightSun *MainLight() const {return _mainLight;}
  void SetMainLight( LightSun *light ) {_mainLight=light;}
  void MainLightChanged(); // fog/light color has been changed
  void RecalculateLighting(float allowCloudShadow, Landscape *land); // fog/light color needs to be changed

#if _ARMA3_NEW_LIGHTING_CFG
  /// current lighting for the scene
  const LandLighting::LightingResult& GetCurrentLighting() const {return _currentLighting;}
#endif

  void BeginSunPixelCounting(int cb);
  void EndSunPixelCounting(int cb);

  void SetTacticalVisibility( float tacVis, float rainRange );
  float GetTacticalVisibility() const {return _tacticalVisibility;}

  float GetSmokeGeneralization() const;

  void LoadConfig(ParamEntryPar cfg,ParamEntryPar fcfg);
  void SaveConfig(ParamFile &cfg) const;

  void SetDefaultSettings();
  void SetDefaultSettingsLevel(int level, bool setClouds=true);
  /// current HW performance level for given settings
  int GetCurrentSettingsLevel() const;

  float GetFogMaxRange() const {return _fogMaxRange;}
  float GetFogMinRange() const {return _fogMinRange;}

  int TacticalFog8( float distSquare ) const {return _tacticalFog(distSquare);}
  int Fog8( float distSquare ) const {return _fog(distSquare);}
  
#if _VBS3_NO_SEABED
  /// returns true if the camera is under the sea surface
  bool UnderWater();
#endif
  void SetSkyColor(ColorVal fogColor, ColorVal skyTopColor);
  
  EngineSceneProperties *GetEngineProperties() const;
  Landscape *GetLandscape() const {return _landscape;}
  ShadowCache &GetShadowCache() const {return _shadowCache;}
  
  /// flush all caches related to the landscape
  void FlushLandCache();

  int PreloadLevelFromDistance2(LODShape *shape, float dist2, float scale);
  int LevelFromDistance2(LODShape *shape, float dist2, float scale)
  {
    int ret = shape->FindLevelWithDensity(dist2,scale,_drawDensity);
    return ret>=0 ? ret : LOD_INVISIBLE;
  }
  int LevelShadowFromDistance2(
    LODShape *shape, float dist2, float scale
    )
  {
    int ret = shape->FindShadowLevelWithDensity(dist2,scale,_drawDensity);
    return ret>=0 ? ret : LOD_INVISIBLE;
  }
  int LevelShadowVolumeFromDistance2(
    LODShape *shape, float dist2, float scale
    )
  {
    int ret = shape->FindShadowVolumeLevelWithDensity(dist2,scale,_drawDensity);
    return ret>=0 ? ret : LOD_INVISIBLE;
  }
  
  /// give the scene opportunity to precalculate needed constants
  void PrepareDraw();
  
  /// start object preparations (encapsulates ObjectsForDrawing...)
  void BeginObjects();
  
  struct ObjectGroup
  {
    const ObjectListFull *_list;
    int _beg;
    int _end;
    
    ObjectGroup(const ObjectListFull *list, int beg, int end):_list(list),_beg(beg),_end(end){}
  };
  /// principal function to submit the object into a scene/lod management
  void ObjectsForDrawing(const ObjectGroup &oGrp, ClipFlags clip, bool shadowPossible);
  /// principal function to submit the object into a scene/lod management
  void ObjectForDrawing(Object *obj, int forceLOD, ClipFlags clip, bool shadowPossible=false);
  /// principal function to submit the object into a scene/lod management
  void ObjectForDrawing(Object *obj, int forceLOD, ClipFlags clip, const PositionRender &position, bool shadowPossible=false);
  /// check if object with given distance and size has a chance to be displayed
  bool CanBeDrawing(float dist2, float estArea) const;
	/// check if object with given position has a chance to occlude
	bool CanDrawOcclusion(const Object* obj, Vector3Par pos) const;

  /// register proxy object for drawing (create a SortObject)
  void ProxyForDrawing(Object *obj, Object *rootObj, int forceLOD, int rootLOD, ClipFlags clip, float dist2, const PositionRender &pos, Object *parentObj = NULL);
  /// register proxy object for drawing, keep reference to object until drawn
  void TempProxyForDrawing(Object *obj, Object *rootObj, int forceLOD, int rootLOD, ClipFlags clip, float dist2, const PositionRender &pos, Object *parentObj = NULL)
  {
    _proxyObjects.Add(obj);
    ProxyForDrawing(obj, rootObj, forceLOD, rootLOD, clip, dist2, pos, parentObj);
  }
  void CloudletForDrawing(Object *obj);
  void ObjectForDrawing(Object *obj)
  {
    ObjectForDrawing(obj,-1,ClipAll,true);
  }

  /// find object used for rendering including instancing
  SortObject *FindSortObject(Object * obj
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawObjId
#endif
    );

  /// called when camera position is changed significantly - used to reset occlusion history
  void OnCameraChanged();
  /// called when camera field of view or orientation is changed abruptly - used to reset LOD memory
  void OnCameraCut();
  
  bool CheckShadowInvisibile(LODShape *shape, Vector3Par pos, float radius) const;

  //! Check the shadow of the object will be visible in the frustum within the specified distance
  bool IsShadowInvisible(float shadowsZ, Vector3Par pos, float dist2, float radius, int shapeSpecial, bool isProxy) const;
private:
  SortObject *ObjectForDrawing(
    Object *obj, int forceLOD, ClipFlags clip,
    bool isProxy, float proxyDist2, const PositionRender &position, int parentCsmLayerReached, const ObjectGroup *oGrp=NULL
  );
  struct SlotObjectsContext;
  struct SplitSlotContext;
  
  void SplitObjectsBySlots(SlotObjectsContext &slotCtx, SplitSlotContext &ctx, Ref<SortObject> *sortObj, int nSortObj);
  void DrawPass1Part(DrawContext &ctx, SortObjectListArg &list, Engine::PredicationMode depthDrawing);
  void DrawPass3Part(Engine::PredicatedCBScope &pred, SortObjectListArg &list, Engine::PredicationMode depthDrawing, bool useSVShadows
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
    );
  void DrawPass3PartBody(SortObjectListArg &list, int nDraw, Engine::PredicationMode depthDrawing, bool useSVShadows
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
    );
  
  void DrawPass4Part(Engine::PredicatedCBScope &pred, SortObjectListArg &list, Engine::PredicationMode depthDrawing, bool useSVShadows
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
    );
  void DrawPass4PartBody(SortObjectListArg &list, int nDraw, Engine::PredicationMode depthDrawing, bool useSVShadows
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
    );

  void AdjustComplexity(DrawContext &ctx, SlotObjectsContext &slotCtx, bool frameRepeated);

  int ComputeTerrainComplexity() const;

  /// helper - derive _minPixelSizeXXX from complexity
  void PixelSizeFromComplexity(float complexity);
  /// compute Square(minPixel) from Square(distance) (using _minPixelSizeXXX)
  float MinPixelSize2FromDist2(float dist2) const;
  
public:
  void EndObjects(); // sort all objects
  //! Go through the _drawShadowProj objects and determine the point closest to sun (using the ShadowDirection vector)
  /** Return the distance from zero of the closest plane */
  float GetSunClosestDistanceFrom0(DrawContext &ctx, int layerIndex);
  /// prepare object lists for rendering, including complexity management
  void DrawPreparation(DrawContext &ctx, float deltaT, bool frameRepeated);
  /// render pass 1 - opaque objects (O - simple opaque, A - with minor alpha faces (house with windows))
  void DrawPass1O(DrawContext &ctx, bool frameRepeated);
  void DrawPass1A(DrawContext &ctx, bool frameRepeated);
  // render objects attached to surface 
  void DrawPassOnSurface(DrawContext &ctx, Engine::PredicationMode depthDrawing);
  
	#ifdef TEST_RAIN_MAP
		void DrawRainMap(DrawContext &ctx);
	#endif
	
	//! Render the shadow buffer
	 void DrawSBShadows(DrawContext &ctx, int layerIndex);
  /// render volume shadows into the stencil
  void DrawVolShadows(DrawContext &ctx
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawObjectsId
#endif
    );
  /// render pass 2 - alpha blended objects
  void DrawPass2(DrawContext &ctx);
  /// render pass 3 - cockpit/1st person
  void DrawPass3(DrawContext &ctx);
  /// render pass 4 - UI
  void DrawPass4(DrawContext &ctx);
  //! Drawing of objects - depth buffer priming
  void DrawDepthPrime(DrawContext &ctx, bool useSVShadows, Engine::PredicationMode pass
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
    );
  /// finish object drawing
  void ObjectsDrawn(DrawContext &ctx
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
    );
  /// clean up object lists after rendering
  void ObjectsCleanUp(DrawContext &ctx, bool cleanUpProxies = true);
  /// clean up light lists after rendering
  void LightsCleanUp();
	
	/// heat haze effect for lights
	bool PrepareLightHeatHaze(HeatHazeRenderArray& instances);

  /// draw diagnostic 3D texts
  void DrawDiagTexts();

  //! Source of the alpha when blending
  void DrawObjectsInstanced(
    int cb, const SortObjectListArg &drawPass, int instancesBeg, int instancesEnd,
    bool allowObjectSort, int level, SortObject::BlendAlphaOrigin bao, bool updateBF, const DrawParameters &dp
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
  );
  void DrawObjectsShadowVolumeInstanced(
    int cb, const SortObjectListArg &drawPass, int instancesBeg, int instancesEnd,
    int level
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
  );
    

  /// shadow factor depending on weather conditions
  float ShadowFactor() const;
  //! Function to determine shadows should be casted or not
  bool CastShadows() const;
  /// return intensity of the flare
  float DrawFlare( ColorVal color, Vector3Par pos, Vector3Par projLightPos, bool secondary, bool sun, float visibility, float flareVisibility);
  /// draw flares, return total intensity
  float DrawFlares(float timeDelta);
	/// light position projection and visibility computation based on screen projection
	Vector3 ProjectLightPos(Vector3Par lightPos);
	//  light position projection
	float GetProjLightVisibility(Vector3Par lightPos, Vector3& projPos);

  //@{ buldozer rendering interface
  void ShowArrow( Vector3Par pos );
  void ShowDiagObject(Object *obj);
  void ShowObject( Object *obj, bool now=false );
  void DrawObject(Object *obj);
  void DiagBBox(const Vector3 *minMax, const Frame &pos, ColorVal color, bool renderNow=false)
  {
    DiagBBox(minMax,pos,color,renderNow ? &Scene::DrawObject : &Scene::ShowDiagObject);
  }
  typedef void (Scene::*RenderAs)(Object *obj);
  void DiagBBox(const Vector3 *minMax, const Frame &pos, ColorVal color, RenderAs render);
  Entity *CreateSelArrow() const;
  Entity *CreateSelSphere() const;
  Entity *CreateSelGizmo() const;
  /// some diagnostics want to be suppressed when other diagnostics are already present
  bool CheckArrows() const {return _arrows.Size()>0;}
  //@}

  //@{ diagnostic rendering interface
  void DrawDiagModel(Vector3Par pos, LODShape *shape, float size=0.1f, ColorVal color=HWhite, bool now=false);
  /// draw colored arrow
  void DrawDiagArrow(Vector3Par pos, Vector3Par dir, float size=0.1f, ColorVal color=HWhite, bool now=false);
  /// draw colored arrow
  void DrawDiagArrow(RString text, Vector3Par pos, Vector3Par dir, float size=0.1f, ColorVal color=HWhite, bool now=false);
  // draw colored sphere
  void DrawDiagRectangle(float size, Vector3 pos, ColorVal color=HWhite, bool now=false);
  void DrawDiagSphere(Vector3Par pos, float size=0.1f, ColorVal color=HWhite, bool now=false);
  void DrawCollisionStar( Vector3Par pos, float size=0.1, ColorVal color=HWhite, bool now=false );
  /// draw a text
  void DrawDiagText(RString text, Vector3Par pos, int priority=INT_MAX, float size=1.0f, ColorVal color=HWhite, float maxDist=FLT_MAX);
  //@}
  /// draw light volumetric or other related effects
  void DrawVolumeLight(
    LODShape *shape, ColorVal color,
    const Frame &pos, float size, bool orthogonalToPlayer
  );

  void DrawFlashLight(LODShape *shape, Vector3Val pos, Vector3Val dir, ColorVal color, Vector3Val scale);
  void DrawIRRay(Vector3Val pos, Vector3Val dir, float size, float dist);
  void AddIRRay(const Person *person, bool add, Vector3Val pos = VZero, Vector3Val dir = VZero);
  void DrawIRRays();

  //! Function calculates the minimal shadow size required (usually for shadow volume purposes)
  float CheckShadowSize(const Vector3 *minMax, Matrix4Par frame, float knownShadowSize2) const;
  //! Function calculates the intersection of the shadow with ground
  /*!
    /param aprox Returned projected position on ground
    /return true in case the intersection occurred, false elsewhere (either straight line didn't intersect or shadow was longer than maxShadow)
  */
  bool ShadowPos(Vector3Par pos, Vector3 &aprox, LightSun *light, float maxShadow) const;
  
  void SetHazeDefaultDistnce(float value) {_hazeDefaultDistance = value;};
  float GetHazeDefaultDistnce() {return _hazeDefaultDistance;};

  LODShape *ForceArrow() const {return Preloaded(ForceArrowModel);}
  LODShape *ForceSphere() const {return Preloaded(SphereModel);}
  LODShape *ForceGizmo() const {return Preloaded(GizmoModel);}

  LODShape *Preloaded( PreloadedShape type ) const
  {
    Assert( type<MaxPreloadedShape );
    return _preloaded[type];
  }
  // TODO: move preloadedTextures out of scene
  Texture *Preloaded( PreloadedTexture type ) const
  {
    return GPreloadedTextures.New(type);
  }
  Texture *Preloaded( RStringB name ) const
  {
    return GPreloadedTextures.New(name);
  }

  //! releases object from _drawObjects
  void ReleaseDrawObject(SortObject *oi);

  //! Calculates a bounding rectangle of a visible land
  void CalculBoundingRect(GridRectangle &brect, float dist, float grid);

	//! Calculates a bounding 2D edges of a visible land
	bool Calcul2DEdges(float dist, float gridSize, int range, bool clampedges, FlexibleLoop2& loop, GridRectangle& brect);
  //! Function to retrieve the material complexity
  //float GetMaterialComplexity() const {return _materialComplexity;}
};

/// functions to obtain default video settings
namespace DefaultVideoOptions
{
  int Complexity();
  int ShadingQuality();
  int ShadowQuality();
  int TextureQuality();
  int HDRQuality();
  int FSAAQuality();
  int VSync();
};

extern Scene *GScene;
#define GLOB_SCENE ( GScene )

#define IS_SHADOW_OBJECT ( GScene ? GScene->GetObjectShadows() : true )

bool CheckAgainstOccluders(const enf::AAB& wbbox);

#endif

