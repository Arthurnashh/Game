#ifdef _MSC_VER
#pragma once
#endif

#ifndef _VIDEO_OPTIONS_CONFIG_HPP
#define _VIDEO_OPTIONS_CONFIG_HPP

#include <Es/Framework/debugLog.hpp>
#include <Es/Strings/rString.hpp>
#include <El/ParamFile/paramFile.hpp>

class VideoOptionsConfig
{
public:

  class MinMaxValue
  {
  public:
    MinMaxValue();
    void LoadFromConfig(ParamEntryPar cfg);
    float _min;
    float _max;

    ClassIsMovableZeroed(MinMaxValue)
  };

  class SimpleValue
  {
  public:
    SimpleValue();
    void LoadFromConfig(ParamEntryPar cfg);

    /// localized text
    RString _text;
    float _value;

    ClassIsMovableZeroed(SimpleValue);
  };
  typedef AutoArray<SimpleValue> TSimpleValueList;

  class TextureValue
  {
  public:
    TextureValue();
    void LoadFromConfig(ParamEntryPar cfg);

    /// localized text
    RString _text;
    /// mipmap bias for textures
    float _mipBias;

    ClassIsMovableZeroed(TextureValue);
  };

  class PPAAValue
  {
  public:
    PPAAValue();
    void LoadFromConfig(ParamEntryPar cfg);

    /// localized text
    RString _text;
    // type of ppaa (0 = disabled, 1 = FXAA, 2 = SMAA)
    int _ppaaType;
    // quality level of ppaa (0 = worst)      
    int _ppaaLevel;

    ClassIsMovableZeroed(PPAAValue);
  };

  class TerrainValue
  {
  public:
    TerrainValue();
    void LoadFromConfig(ParamEntryPar cfg);

    /// localized text
    RString _text;
    // terrain grid
    float _value;

    ClassIsMovableZeroed(TerrainValue);
  };

  class ShadowValue
  {
  public:
    ShadowValue();
    void LoadFromConfig(ParamEntryPar cfg);

    /// localized text
    RString _text;
    // type of shadows (0 = disabled, 1 = stencil, 2,3,4 = shadow buffer)
    int _shadowType;

    ClassIsMovableZeroed(ShadowValue);
  };

  class ATOCValue
  {
  public:
    ATOCValue();
    void LoadFromConfig(ParamEntryPar cfg);

    bool IsATOCEnabled() const
    {
      return _grass || _oldTrees || _newTrees || _other;
    }

    int Distance(const ATOCValue& other);

    /// localized text
    RString _text;
    // ATOC for grass
    bool _grass;
    // ATOC for old tree shader
    bool _oldTrees;
    // ATOC for new tree shader
    bool _newTrees;
    // ATOC for other shaders (SuperHair, SuperATOC)
    bool _other;

    enum AToCMask
    {
      AToC_Disable = 0,
      AToC_Grass = 1,
      AToC_TreeNew = 2,
      AToC_TreeOld = 4,
      AToC_Other = 8,
      AToC_EnableAll = 255
    };
    static AToCMask ConvertATOCValueToMask(const ATOCValue& val)
    {
      int mask = 0;
      if (val._grass)
        mask = mask | AToC_Grass;
      if (val._oldTrees)
        mask = mask | AToC_TreeOld;
      if (val._newTrees)
        mask = mask | AToC_TreeNew;
      if (val._other)
        mask = mask | AToC_Other;
     
      return (AToCMask)mask;
    }
    
    static ATOCValue ConvertATOCMaskToValue(int mask)
    {
      ATOCValue val;
      val._grass = ((mask & AToC_Grass) != 0);
      val._oldTrees = ((mask & AToC_TreeOld) != 0);
      val._newTrees = ((mask & AToC_TreeNew) != 0);
      val._other = ((mask & AToC_Other) != 0);
      return val;
    }

    ClassIsMovableZeroed(ATOCValue);
  };

  template<class T>
  class ValueList
  {
  public:
    int GetSize() const {return _values.Size();}
    const T& GetValue(int index) const
    {
      DoAssert(index >= 0 && index < _values.Size());
      if (index >= 0 && index < _values.Size())
        return _values[index];      
      else
        return NullValue;
    }
    RString GetText(int index) const
    {
      DoAssert(index >= 0 && index < _values.Size());
      if (index >= 0 && index < _values.Size())
        return _values[index]._text;

      return RString();
    }

    /// find item with nearest value
    int FindNearest(float value) const
    {
      float bestDiff = FLT_MAX;
      int bestIndex = -1;
      for (int i=0; i<_values.Size(); ++i)
      {
        float diff = fabs(value - _values[i]._value);
        if (diff < bestDiff)
        {
          bestDiff = diff;
          bestIndex = i;
        }
      }

      return bestIndex;
    }

    void LoadFromConfig(ParamEntryPar cfg)
    {
      if (cfg.GetEntryCount() == 0)
        return;
      _values.Clear();
      _values.Reserve(cfg.GetEntryCount());
      for (int i=0; i<cfg.GetEntryCount(); i++)
      {
        ParamEntryVal entry = cfg.GetEntry(i);
        if (entry.IsClass())
        {
          int idx = _values.Add();
          T& val = _values[idx];
          val.LoadFromConfig(entry);
        }
      }
    }

  private:
    T NullValue;
    AutoArray<T> _values;
  };

  class OverallQualityItem
  {
  public:

    OverallQualityItem();
    /**
    \param videoOptionsCfg cfg for all video options
    \param cfg cfg for this item
    */
    void LoadFromConfig(ParamEntryPar videoOptionsCfg, ParamEntryPar cfg);
    void CreateDefault();

    /// returns how much different we are from the other otem 
    /// 0.0f means we are the same
    float GetDifferenceFrom(const OverallQualityItem& other);

    /// localized text
    RString _text;

    float _visibility;
    float _objectsVisibility;
    float _shadowsVisibility;

    SimpleValue _hdr;  //< bits per color (8 or 16)
    int _hdrIndex; // index to array of values in config

    PPAAValue _ppaa; 
    int _ppaaIndex; // index to array of values in config

    SimpleValue _anisotropicFiltering; //< max aniso value
    int _anisotropicFilteringIndex; // index to array of values in config

    SimpleValue _pip; //< distance in pip cameras
    int _pipIndex; // index to array of values in config

    SimpleValue _fsaa; // antialiasing
    int _fsaaIndex; // index to array of values in config

    TextureValue _textureQuality; //< mipmap bias
    int _textureQualityIndex; // index to array of values in config

    SimpleValue _objectQuality; //< target scene complexity
    int _objectQualityIndex; // index to array of values in config

    TerrainValue _terrainQuality; 
    int _terrainQualityIndex; // index to array of values in config

    SimpleValue _cloudQuality; //< number of simul weather layers (0 = disabled)
    int _cloudQualityIndex; // index to array of values in config

    ShadowValue _shadowQuality; 
    int _shadowQualityIndex; // index to array of values in config

    ATOCValue _atocQuality;
    int _atocQualityIndex;

    // postprocess settings
#if _ARMA3_SEPARATED_PP_SETTINGS
    SimpleValue _ppSSAOQuality;
    int _ppSSAOQualityIndex;
    SimpleValue _ppCausticsQuality;
    int _ppCausticsQualityIndex;
    float _ppBloom;
    float _ppDOF;
    float _ppRotBlur;
    float _ppRadialBlur;
#else
    int _postProcessQuality;
#endif
    ClassIsGeneric(OverallQualityItem);
  };

  /// info about benchmark
  class Benchmark
  {
  public:
    Benchmark();
    void LoadFromConfig(ParamEntryPar cfg);

    // default result is returned when benchmark fails or is disabled
    // this is a time in ms (duration of benchmark)
    float _defaultResult;

    // This determines how to obtain overall setting based on benchmark result.
    // Values are times in ms (benchmark avg duration).
    // if result > firstValue, overallQuality = VeryLow
    // else if result > secondValue, overallQuality = Low
    // etc...
    // Values were obtained from benchmark result on various GPUs (best GPUs are around 60ms, mediocre around 130ms)
    AutoArray<float> _resultLimits;
  };

  void LoadFromConfig(ParamEntryPar cfg);

  /** 
  Tries to detect current overall settings.
  \param nearest finds nearest overall settings (otherwise settings must match perfectly)
  \return index to _overallSettings, or -1 if no overall setting matches current settings   
  */
  int DetectCurrentOverallSettings(bool nearest);

  MinMaxValue _visibility; //< overall visibility in m
  MinMaxValue _objectsVisibility; //< objects visibility in m
  MinMaxValue _shadowsVisibility; //< shadows visibility in m
  ValueList<SimpleValue> _hdr; //< bits per color (8 or 16)
  ValueList<PPAAValue> _ppaa;
  ValueList<SimpleValue> _fsaa;
  ValueList<SimpleValue> _anisotropicFiltering; //< max aniso value
  ValueList<SimpleValue> _pip; //< distance in pip cameras
  ValueList<TextureValue> _textureQuality; //< mipmap bias
  ValueList<SimpleValue> _objectQuality; //< target scene complexity
  ValueList<TerrainValue> _terrainQuality;
  ValueList<SimpleValue> _cloudQuality; //< number of simul weather layers (0 = disabled)
  ValueList<ShadowValue> _shadowQuality;
  ValueList<ATOCValue> _atocQuality;

#if _ARMA3_SEPARATED_PP_SETTINGS
  // postprocess settings
  ValueList<SimpleValue> _ppSSAOQuality;
  ValueList<SimpleValue> _ppCausticsQuality;
  MinMaxValue _ppBloom;
  MinMaxValue _ppDOF;
  MinMaxValue _ppRotBlur;
  MinMaxValue _ppRadialBlur;
#endif

  OverallQualityItem _defaultSettings;
  AutoArray<OverallQualityItem> _overallSettings;
  Benchmark _benchmark;
};

#endif //_VIDEO_OPTIONS_CONFIG_HPP