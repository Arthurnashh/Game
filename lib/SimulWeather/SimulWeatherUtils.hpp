//=====================================================================================
// Various utilities for Simul Weather integration
// \author Luke
//=====================================================================================

#ifdef _MSC_VER
#pragma once
#endif

#ifndef _ARMA3_SIMULWEATHERUTILS_HPP
#define _ARMA3_SIMULWEATHERUTILS_HPP

#if _ARMA3_SIMUL_WEATHER

#include "SimulWeatherIncludes.hpp"
#include "El/ParamFile/paramFile.hpp"

class SimulSkyRendererBase;
class SimulCloudRendererBase;

namespace SimulWeatherUtils
{
  /** keyframer used just to load keyframes */
  class SkyKeyframerLoad : public simul::sky::BaseKeyframer, public RefCount
  {
  public:
    std::istream &Load(std::istream &is);

    //@{ implementation of BaseKeyframer
    virtual simul::sky::BaseKeyframe *InsertKeyframe(float t);
    virtual void RestartNextKeyframe() {}
    //@}
  };
  class CloudKeyframerLoad : public simul::sky::BaseKeyframer, public RefCount
  {
  public:
    std::istream &Load(std::istream &is);
    //@{ implementation of BaseKeyframer
    virtual simul::sky::BaseKeyframe *InsertKeyframe(float t);
    virtual void RestartNextKeyframe() {}
    //@}
  };

  /** represents cache of loaded sequencer files */
  class SequencerFileCache
  {
  public:
    // max possible items in cache
    static const int MAX_ITEMS = 10;

    /** one item of the cache */
    struct CacheItem
    {
    public:
      CacheItem()
      {
        _cloudKeyframer = new CloudKeyframerLoad();
        _skyKeyframer = new SkyKeyframerLoad();
      }

      RString _fileName;
      Ref<CloudKeyframerLoad> _cloudKeyframer;
      Ref<SkyKeyframerLoad> _skyKeyframer;
    };

    CacheItem* GetItem(int index)
    {
      if (index >= 0 && index < _items.Size())
        return &_items[index];

      return 0;
    }

    CacheItem* GetItem(RString fileName)
    {
      int index = LoadFile(fileName);
      if (index >= 0 && index < _items.Size())
        return GetItem(index);

      return 0;
    }

    // clears cache
    void Clear()
    {
      _items.Clear();
    }

  private:
    /// loads sequencer file to cache (if not already loaded), returns id in cache (or -1 if not loaded)
    int LoadFile(RString fileName, bool forceReload = false);
    /// returnd index of sequencer file (-1 if not found)
    int FindFile(RString fileName);

    /// we are using simple array, not HashMap, because there will be just a few different sequencer files in the game
    AutoArray<CacheItem> _items;

  };


  /** tries to fill sky keyframe and cloud keyframe from given config */
  extern bool LoadKeyframes(ParamEntryPar entry, simul::sky::SkyKeyframer::SkyKeyframe &skyKeyframe, 
    simul::clouds::CloudKeyframer::Keyframe &cloudKeyframe);

  /** tries to fill sky keyframe and cloud keyframe from given sequencer file and given keyframe in this file
  \param fileName name of the sequencer file
  \param keyframe number of the keyframe in the sequencer file (starts at 0)
  */
  extern bool LoadKeyframes(RString fileName, int keyframe, simul::sky::SkyKeyframer::SkyKeyframe &skyKeyframe, 
    simul::clouds::CloudKeyframer::Keyframe &cloudKeyframe);

  /** Tries to load sequencer file 
  either skyRenderer of skyKeyframer must be specified
  either cloudRenderer of cloudKeyframer must be specified
  */
  extern bool LoadSequencerFile(RString fileName, SimulSkyRendererBase* skyRenderer, SimulCloudRendererBase* cloudRenderer, 
    SkyKeyframerLoad* skyKeyframer = 0, CloudKeyframerLoad* cloudKeyframer = 0);

  /** fills result with interpolation of k1 and k2 based on interp */
  extern void InterpolateSkyKeyframes(float interp, const simul::sky::SkyKeyframer::SkyKeyframe &k1, 
    const simul::sky::SkyKeyframer::SkyKeyframe &k2, simul::sky::SkyKeyframer::SkyKeyframe &result);

  /** fills result with interpolation of k1 and k2 based on interp */
  extern void InterpolateCloudKeyframes(float interp, const simul::clouds::CloudKeyframer::Keyframe &k1, 
    const simul::clouds::CloudKeyframer::Keyframe &k2, simul::clouds::CloudKeyframer::Keyframe &result);
  
  /** vector conversion functions */
  extern void Convert(const simul::math::Vector3& from, Vector3& to, bool swapYZ = false);
  extern void Convert(const Vector3& from, simul::math::Vector3& to, bool swapYZ = false);
  
  extern SimulWeatherUtils::SequencerFileCache& GetSequencerFileCache(); 

}//namespace SimulWeatherUtils

// must be defined outside of namespace

TypeIsGeneric(SimulWeatherUtils::SequencerFileCache::CacheItem);

#endif //_ARMA3_SIMUL_WEATHER
#endif //_ARMA3_SIMULWEATHERUTILS_HPP
