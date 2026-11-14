#ifdef _MSC_VER
#pragma once
#endif

#ifndef _ARMA3_SIMULSKYRENDERERBASE_HPP
#define _ARMA3_SIMULSKYRENDERERBASE_HPP

#if _ARMA3_SIMUL_WEATHER

#include "SimulWeatherIncludes.hpp"
#include "El/ParamFile/paramFile.hpp"

/**
Base class for Simul Weather sky renderers used in Arma
*/
class SimulSkyRendererBase: public simul::sky::BaseSkyRenderer
{
public:
  /// number of textures we need (2 to interpolate, 1 to fill)
  static const int TextureCount = 3;

  /**
  \param renderSky if we want to render the sky (otherwise old Arma skybox is used) 
  \param useFadeTextures if we want to use fade textures (used for cloud distance fading)
  \param useColourSky if we want to use coulour sky instead of normal sky
  */
  SimulSkyRendererBase(bool renderSky = false, bool useFadeTextures = true, bool useColourSky = false);
  virtual ~SimulSkyRendererBase();

  //@{ Implementation of SkyTexturesCallback
  virtual void SetSkyTextureSize(unsigned size);
  virtual void SetFadeTextureSize(unsigned width,unsigned height,unsigned num_alts);
  virtual void FillFadeTextureBlocks(int,int,int,int,int,int,int,
    const float *,
    const float *);
  //@}

  //@{ Implementation of BaseSkyRenderer
  //! Render a planet with texture \a tex, angular radius \a rad radians, in direction \a dir (x,y,z), with colour \a colr.
  //! If \a do_lighting is true, the planet will be directionally-lit - e.g. moon phases.
  virtual bool RenderPlanet(void* tex,float rad,const float *dir,const float *colr,bool do_lighting)
  {
    // SIMUL WEATHER TODO
    return false;
  }
  virtual bool IsYVertical() 
  { 
    /// we use y as vertical axis in Arma
    return true;
  }
  //@}


  /// initializes renderer (checks for formats, creates resouorces,...), returns if succeeded
  bool Init(); 
  /// deinitializes renderer (checks for formats, creates resouorces,...)
  void DeInit(); 

  /// performs simulation 
  virtual void Simulate(float deltaT);

  bool GetRenderSky() const {return _renderSky;}

  void SetUseFadeTextures(bool use) {_useFadeTextures = use;}
  bool GetUseFadeTextures() const {return _useFadeTextures;}
  bool GetUseColourSky() const {return _useColourSky;}
  bool GetInitialized() const {return _initialized;}
 
  float GetFadeInterp() const;

  /// setups auto brightness for keyframes
  void SetAutoBrightness(bool enable, float strength);

  /// initializes from cfg
  void InitFromCfg(ParamEntryPar entry);
  /// setups keyframes based on given keyframe
  void InitKeyframes(int numKeyframes, simul::sky::SkyKeyframer::SkyKeyframe& keyframe);

  /// applies values of src keyframe to dest keyframe (not all values - we don't want to overwrite daytime for example)
  void ApplyKeyframe(simul::sky::SkyKeyframer::SkyKeyframe& dest, const simul::sky::SkyKeyframer::SkyKeyframe& src, float overcast);

  /// synchronizes arma's time and position with simul weather
  void SynchronizeTimeAndPlace();

private:
  /// initializes renderer - creates all resources,..., returns if succeeded
  virtual bool InitRenderer() = 0;
  /// deinitializes renderer - destroys all resources,...
  virtual void DeInitRenderer() = 0;
  /// creates textures used for fading
  virtual void CreateFadeTextures() = 0;
  /// creates textures used for sky rendering
  virtual void CreateSkyTextures() = 0;
  /// creates sunlight textures
  virtual void CreateSunlightTextures() = 0;


  /// if we want to render the sky
  bool _renderSky;
  /// flag that tells us if we want to use fade textures for cloud fading (otherwise it is computed on CPU)
  bool _useFadeTextures;
  /// if we want to use colour sky instead of normal sky
  bool _useColourSky;

  /// if we should use auto brightness computation
  bool _useAutoBrightness;

  /// if renderer was successfully initialized 
  bool _initialized;
};

#endif //_ARMA3_SIMUL_WEATHER
#endif //_ARMA3_SIMULSKYRENDERERBASE_HPP