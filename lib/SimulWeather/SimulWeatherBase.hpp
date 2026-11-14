//=====================================================================================
// Wrapper classes for simul weather functionality
// \author Luke
//=====================================================================================

#ifdef _MSC_VER
#pragma once
#endif

#ifndef _ARMA3_SIMULWEATHERBASE_HPP
#define _ARMA3_SIMULWEATHERBASE_HPP

#if _ARMA3_SIMUL_WEATHER

#include "SimulSkyRendererBase.hpp"
#include "SimulCloudRendererBase.hpp"
#include "El/ParamFile/paramFile.hpp"
#include "El/ParamArchive/serializeClass.hpp"

class Engine;

/** one item that defines specific overcast configuration */
class SimulOvercastItem
{
public:
  SimulOvercastItem() : _overcast(0.0f), _valid(false) {}

  void Load(ParamEntryPar entry);

  // overcast value
  float _overcast;

  // if item is valid
  bool _valid;

  // keyframes for this overcast
  simul::sky::SkyKeyframer::SkyKeyframe _skyKeyframe;
  simul::clouds::CloudKeyframer::Keyframe _cloudKeyframe; 
};

TypeIsGeneric(SimulOvercastItem);

/**
Base class that wraps simul weather functionality
*/
class SimulWeatherBase: public RefCount, public SerializeClass
{
public:
  /// we added simul weather to world save version 26
  static const int SimulWeatherWorldSerializeVersion = 26;

  SimulWeatherBase(Engine* engine);
  virtual ~SimulWeatherBase();

  /// performs simulation 
  virtual void Simulate(float deltaT);
  /// performs rendering
  virtual void Render(bool cubemap = false);

  /// diag drawing
  virtual void DiagDraw();

  /// performs initialization (creates renderers)
  virtual void Init();
  /// performs initialization of resources
  virtual void InitResources();
  /// performs deinitialization of resources
  virtual void DeInitResources();

  /** initializes from given config */
  void InitFromCfg(ParamEntryPar entry);

  /// recalculates lighting for clouds
  void RecalculateLighting();

  void SetRenderMode(SimulCloudRendererBase::RenderMode mode);

  /// loads sequencer file and sets keyframes
  bool LoadSequencerFile(RString fileName);

  /// initializes all cloud and sky keyframes with keyframe corresponding to given overcast (0-1)
  void InitKeyframes(float overcast);

  /// fills values for keyframes based on given overcast
  void GetKeyframesForOvercast(float overcast, simul::sky::SkyKeyframer::SkyKeyframe& skyKeyframe, simul::clouds::CloudKeyframer::Keyframe& cloudKeyframe);

  /// this is called when weather is reseted (most probably at the world initialization)
  void ResetWeather();

  /// gets cloud occlusion between two points in 3D space (0 = no clouds, 1 = full clouds)
  float GetCloudOcclusion(const Vector3& startPoint, const Vector3& endPoint);

  /// position of given point relative to cloud grid
  enum EPointCloudPos
  {
    PCPBelow, //< point is below cloud grid
    PCPInside, //< point is inside cloud grid
    PCPAbove, //< point is above cloud grid
  };
  /// gets position of given point relative to cloud grid
  EPointCloudPos GetPointPosition(const Vector3& point);
  /// gets cloud density at given point (0 = no cloud)
  float GetCloudDensity(const Vector3& point);

  // LUKE - we have to use simul weather smart ptrs, not arma Refs (because of PrepareToDestroy functions - they are called
  // only when SmartPtr is used
  simul::base::SmartPtr<SimulCloudRendererBase> GetCloudRenderer() {return _cloudRenderer;}
  simul::base::SmartPtr<SimulSkyRendererBase> GetSkyRenderer() {return _skyRenderer;}

  //! serialize all values into current class
  virtual LSError Serialize(ParamArchive &ar);

  //! initializes all needed after load
  void InitAfterLoad();

  /** synchronizes keyframes with arma overcast
  \param updateCurrentKeyframes - if we can also update current keyframes (this will cause slowdown - all tables will be recomputed)
  */
  void SynchronizeKeyframes(bool updateCurrentKeyframes);
protected:
  /// sky renderer
  simul::base::SmartPtr<SimulSkyRendererBase> _skyRenderer;
  /// cloud renderer
  simul::base::SmartPtr<SimulCloudRendererBase> _cloudRenderer;

  /// pointer to engine
  Engine* _engineBase;

private:
  /// creates renderers
  virtual void CreateRenderers() = 0;
  /// updates cloud renderer based on sky renderer in Simulate
  virtual void UpdateSkyAndCloudHookup() = 0;
  /// connects cloud and sky renderer
  void ConnectInterfaces();

  /// initializes default keyframes (in case they are not loaded correctly)
  void InitDefaultKeyframes();

  /** updates keyframes based on current wanted overcast
  \param updateCurrentKeyframes - if updating of current keyframes is allowed (this can really slow game - because all tables have to be recomputed),
                                  use this only when not in game
  */
  void UpdateWantedOvercast(bool updateCurrentKeyframes);

  /** applies _keyframeOvercast to keyframes */
  void ApplyKeyframeOvercast();

  typedef AutoArray<SimulOvercastItem> TOvercastList;

  /// list of overcast configurations
  TOvercastList _overcastList;
  /// default keyframes
  simul::sky::SkyKeyframer::SkyKeyframe _defaultSkyKeyframe;
  simul::clouds::CloudKeyframer::Keyframe _defaultCloudKeyframe; 

  /// number of keyframes per day
  int _numKeyframesPerDay;

  /// last time we synchronized keyframes
  float _lastSynchronizeTime;

  /// values that were last set in World, when we were simulating SimulWeather
  float _lastActualOvercast;
//  float _lastNextWeatherChangeOvercast;
  float _lastWantedOvercast;

  /// list of overcast that were set to keframes during last synchronization
  AutoArray<float> _keyframeOvercast;
};


#endif //_ARMA3_SIMUL_WEATHER
#endif //_ARMA3_SIMULWEATHERBASE_HPP
