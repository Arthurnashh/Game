#include "wpch.hpp"
#include "videoOptionsConfig.hpp"
#include "scene.hpp"
#include "engine.hpp"

VideoOptionsConfig::MinMaxValue::MinMaxValue()
  : _min(0.0f), _max(0.0f) 
{}

void VideoOptionsConfig::MinMaxValue::LoadFromConfig(ParamEntryPar cfg)
{
  _min = cfg >> "minValue";
  _max = cfg >> "maxValue";
}

VideoOptionsConfig::SimpleValue::SimpleValue()
  : _value(0.0f) 
{}

void VideoOptionsConfig::SimpleValue::LoadFromConfig(ParamEntryPar cfg)
{
  _text = cfg >> "text";
  _value = cfg >> "value";
}

VideoOptionsConfig::TextureValue::TextureValue()
  : _mipBias(0.0f)
{}

void VideoOptionsConfig::TextureValue::LoadFromConfig(ParamEntryPar cfg)
{
  _text = cfg >> "text";
  _mipBias = cfg >> "mipBias";
}

VideoOptionsConfig::PPAAValue::PPAAValue()
  : _ppaaType(0), _ppaaLevel(0) 
{}

void VideoOptionsConfig::PPAAValue::LoadFromConfig(ParamEntryPar cfg)
{
  RString textMethod = cfg >> "textMethod";
  RString textQuality = cfg >> "textQuality";

  _text = textMethod + " " + textQuality;
  _ppaaType = cfg >> "ppaaType";
  _ppaaLevel = cfg >> "ppaaLevel";
}

VideoOptionsConfig::TerrainValue::TerrainValue()
  : _value(12.5f) 
{}

void VideoOptionsConfig::TerrainValue::LoadFromConfig(ParamEntryPar cfg)
{
  _text = cfg >> "text";
  _value = cfg >> "terrainGrid";
}

VideoOptionsConfig::ShadowValue::ShadowValue()
  : _shadowType(0)
{}

void VideoOptionsConfig::ShadowValue::LoadFromConfig(ParamEntryPar cfg)
{
  _text = cfg >> "text";
  _shadowType = cfg >> "shadowType";
}

VideoOptionsConfig::ATOCValue::ATOCValue()
  :_grass(false), _oldTrees(false), _newTrees(false), _other(false)
{}

void VideoOptionsConfig::ATOCValue::LoadFromConfig(ParamEntryPar cfg)
{
  _text = cfg >> "text";
  _grass = cfg >> "grass";
  _oldTrees = cfg >> "oldTrees";
  _newTrees = cfg >> "newTrees";
  _other = cfg >> "other";
}

int VideoOptionsConfig::ATOCValue::Distance(const ATOCValue& other)
{
  int result = 0;
  result += (_grass != other._grass) ? 1 : 0;
  result += (_oldTrees != other._oldTrees) ? 1 : 0;
  result += (_newTrees != other._newTrees) ? 1 : 0;
  result += (_other != other._other) ? 1 : 0;
  return result;
}


VideoOptionsConfig::OverallQualityItem::OverallQualityItem()
{
  CreateDefault();
}

void VideoOptionsConfig::OverallQualityItem::CreateDefault()
{
  // fill some meaningful default values
  _text = "Default";
  _visibility = 1600;
  _objectsVisibility = 1200;
  _shadowsVisibility = 100;
  
  _hdr._value = 16;
  _hdrIndex = -1;

  _ppaa._ppaaType = 0;
  _ppaa._ppaaLevel = 0;
  _ppaaIndex = -1;
  _fsaaIndex = -1;

  _anisotropicFiltering._value = 4;
  _anisotropicFilteringIndex = -1;

  _pip._value = 500;
  _pipIndex = -1;

  _textureQuality._mipBias = 2;
  _textureQualityIndex = -1;

  _objectQuality._value = 500000;
  _objectQualityIndex = -1;

  _terrainQuality._value = 12.5f;
  _terrainQualityIndex = -1;

  _cloudQuality._value = 64;
  _cloudQualityIndex = -1;

  _shadowQuality._shadowType = 2;
  _shadowQualityIndex = -1;

  _atocQuality = ATOCValue();
  _atocQualityIndex = -1;

#if _ARMA3_SEPARATED_PP_SETTINGS
  _ppSSAOQuality._value = 0;
  _ppSSAOQualityIndex = 0;
  _ppCausticsQuality._value = 0;
  _ppCausticsQualityIndex = 0;
  _ppBloom = 1;
  _ppDOF = 1;
  _ppRotBlur = 1;
  _ppRadialBlur = 1;
#else
  _postProcessQuality = 0;
#endif
}

/// helper for loading overall item cfg
template<class T>
static void OverallItemLoadHelper(ParamEntryPar videoOptionsCfg, ParamEntryPar cfg, T& val, int& valIdx, const char *paramName)
{
  RString settingName = cfg >> paramName;

  valIdx = -1;

  ParamEntryPtr entry = videoOptionsCfg.FindEntry(paramName);
  if (entry.IsNull())
  {
    LogF("VideoOptionsConfig - can not find class \"%s\" in CfgVideoOptions!", cc_cast(paramName));
    return;
  }

  ParamEntryPtr settingEntry = entry->FindEntry(settingName);
  if (settingEntry.IsNull())
  {
    LogF("VideoOptionsConfig - can not find class \"%s\" in \"%s\" in CfgVideoOptions!",
      cc_cast(settingName), cc_cast(paramName));
    return;
  }

  // find idx
  for (int i=0; i<entry->GetEntryCount(); ++i)
  {
    if (entry->GetEntry(i).GetName() == settingEntry->GetName())
    {
      valIdx = i;
      break;
    }
  }

  Assert(valIdx >= 0);

  val.LoadFromConfig(*settingEntry);
}

void VideoOptionsConfig::OverallQualityItem::LoadFromConfig(ParamEntryPar videoOptionsCfg, ParamEntryPar cfg)
{
  if (cfg.GetEntryCount() == 0)
    return;

  _text = cfg >> "text";

  _visibility = cfg >> "visibility";
  _objectsVisibility = cfg >> "objectVisibility";
  _shadowsVisibility = cfg >> "shadowVisibility";

  OverallItemLoadHelper(videoOptionsCfg, cfg, _hdr, _hdrIndex, "hdr");
  OverallItemLoadHelper(videoOptionsCfg, cfg, _ppaa, _ppaaIndex, "ppaa");
  OverallItemLoadHelper(videoOptionsCfg, cfg, _fsaa, _fsaaIndex, "fsaa");
  OverallItemLoadHelper(videoOptionsCfg, cfg, _anisotropicFiltering, _anisotropicFilteringIndex, "anisotropicFiltering");
  OverallItemLoadHelper(videoOptionsCfg, cfg, _pip, _pipIndex, "pip");
  OverallItemLoadHelper(videoOptionsCfg, cfg, _textureQuality, _textureQualityIndex, "textureQuality");
  OverallItemLoadHelper(videoOptionsCfg, cfg, _objectQuality, _objectQualityIndex, "objectsQuality");
  OverallItemLoadHelper(videoOptionsCfg, cfg, _terrainQuality, _terrainQualityIndex, "terrainQuality");
  OverallItemLoadHelper(videoOptionsCfg, cfg, _cloudQuality, _cloudQualityIndex, "cloudQuality");
  OverallItemLoadHelper(videoOptionsCfg, cfg, _shadowQuality, _shadowQualityIndex, "shadowQuality");
  OverallItemLoadHelper(videoOptionsCfg, cfg, _atocQuality, _atocQualityIndex, "atocQuality");

  // postprocesses
#if _ARMA3_SEPARATED_PP_SETTINGS
  OverallItemLoadHelper(videoOptionsCfg, cfg, _ppSSAOQuality, _ppSSAOQualityIndex, "ppSSAOQuality");
  OverallItemLoadHelper(videoOptionsCfg, cfg, _ppCausticsQuality, _ppCausticsQualityIndex, "ppCausticsQuality");
  _ppBloom = cfg >> "ppBloom";
  _ppDOF = cfg >> "ppDOF";
  _ppRotBlur = cfg >> "ppRotBlur";
  _ppRadialBlur = cfg >> "ppRadialBlur";
#else
  _postProcessQuality= cfg >> "postProcessQuality";  
#endif
}

float VideoOptionsConfig::OverallQualityItem::GetDifferenceFrom(const OverallQualityItem& other)
{
  // this is kind of chaotic, because sometimes we must compare index and sometimes value
  // (because each option is set in different way)
  // also see VideoOptionsConfig::DetectCurrentOverallSettings how currentSettings are filled

  float difference = 0.0f;

  auto distanceDiffFnc = [](float d1, float d2) -> float {
    return int(fabs(log(d1) - log(d2)));
  };
  auto shadowDistanceDiffFnc = [](float d1, float d2) -> float {
    return int(fabs(d1 - d2) / 50);
  };
  auto indexDistanceFnc = [](int i1, int i2) -> float {
    return abs(i1 - i2);
  };
  auto atocDistanceFnc = [](const ATOCValue& val1, const ATOCValue& val2) -> float {
    if (val1.IsATOCEnabled() == val2.IsATOCEnabled())
      return 0.0f;
    else
      return 1.0f;
  };
#if _ARMA3_SEPARATED_PP_SETTINGS
  auto postProcessDistanceFnc = [](float v1, float v2) -> float {
    // for postprocesses only enabled/disabled is important
    int enabled1 = v1 > 0.001f ? 1 : 0;
    int enabled2 = v2 > 0.001f ? 1 : 0;
    return abs(enabled1 - enabled2);
  };
#endif
  
  // TODO - probably remove distances from checking!
  difference += distanceDiffFnc(_visibility,other._visibility);
  difference += distanceDiffFnc(_objectsVisibility, other._objectsVisibility);
  difference += shadowDistanceDiffFnc(_shadowsVisibility, other._shadowsVisibility);
  
  difference += indexDistanceFnc(_hdrIndex, other._hdrIndex);
  difference += indexDistanceFnc(_ppaaIndex, other._ppaaIndex);
  difference += indexDistanceFnc(_fsaaIndex, other._fsaaIndex);
  difference += indexDistanceFnc(_anisotropicFilteringIndex, other._anisotropicFilteringIndex);
#if _HELISIM_RENDER_TO_TEXTURE
  difference += indexDistanceFnc(_pipIndex, other._pipIndex);
#endif
  difference += indexDistanceFnc(_textureQualityIndex, other._textureQualityIndex);
  difference += indexDistanceFnc(_objectQualityIndex, other._objectQualityIndex);
  difference += indexDistanceFnc(_terrainQualityIndex, other._terrainQualityIndex);
#if _ARMA3_SIMUL_WEATHER
  difference += indexDistanceFnc(_cloudQualityIndex, other._cloudQualityIndex);
#endif
  difference += indexDistanceFnc(_shadowQualityIndex, other._shadowQualityIndex);

  difference += atocDistanceFnc(_atocQuality, other._atocQuality);

#if _ARMA3_SEPARATED_PP_SETTINGS
  difference += indexDistanceFnc(_ppSSAOQualityIndex, other._ppSSAOQualityIndex);
  difference += indexDistanceFnc(_ppCausticsQualityIndex, other._ppCausticsQualityIndex);
  difference += postProcessDistanceFnc(_ppBloom, other._ppBloom);
  difference += postProcessDistanceFnc(_ppDOF, other._ppDOF);
  difference += postProcessDistanceFnc(_ppRotBlur, other._ppRotBlur);
  difference += postProcessDistanceFnc(_ppRadialBlur, other._ppRadialBlur);
#else
  difference += indexDistanceFnc(_postProcessQuality, other._postProcessQuality);
#endif

  return difference;
}

VideoOptionsConfig::Benchmark::Benchmark()
  :_defaultResult(1000.0f)
{
  
}

void VideoOptionsConfig::Benchmark::LoadFromConfig(ParamEntryPar cfg)
{
  _defaultResult = cfg >> "defaultResult";
  ParamEntryVal limits = cfg >> "resultLimits";
  _resultLimits.Clear();
  for (int i=0; i<limits.GetSize(); ++i)
  {
    float limit = limits[i];
    _resultLimits.Add(limit);
  }

}

void VideoOptionsConfig::LoadFromConfig(ParamEntryPar cfg)
{
  _visibility.LoadFromConfig(cfg >> "Visibility");
  _objectsVisibility.LoadFromConfig(cfg >> "ObjectsVisibility");
  _shadowsVisibility.LoadFromConfig(cfg >> "ShadowsVisibility");
  _hdr.LoadFromConfig(cfg >> "HDR");
  _ppaa.LoadFromConfig(cfg >> "PPAA");
  _fsaa.LoadFromConfig(cfg >> "FSAA");
  _anisotropicFiltering.LoadFromConfig(cfg >> "AnisotropicFiltering");
  _pip.LoadFromConfig(cfg >> "PiP");
  _textureQuality.LoadFromConfig(cfg >> "TextureQuality");
  _objectQuality.LoadFromConfig(cfg >> "ObjectsQuality");
  _terrainQuality.LoadFromConfig(cfg >> "TerrainQuality");
  _cloudQuality.LoadFromConfig(cfg >> "CloudQuality");
  _shadowQuality.LoadFromConfig(cfg >> "ShadowQuality");
  _atocQuality.LoadFromConfig(cfg >> "ATOCQuality");

  // postprocesses
#if _ARMA3_SEPARATED_PP_SETTINGS
  _ppBloom.LoadFromConfig(cfg >> "PPBloom");
  _ppDOF.LoadFromConfig(cfg >> "PPDOF");
  _ppRotBlur.LoadFromConfig(cfg >> "PPRotBlur");
  _ppRadialBlur.LoadFromConfig(cfg >> "PPRadialBlur");
#endif

  _defaultSettings.LoadFromConfig(cfg, cfg >> "DefaultSettings");
  ParamEntryVal overallCfg = cfg >> "OverallSettings";

  if (overallCfg.GetEntryCount() > 0)
  {
    _overallSettings.Clear();
    _overallSettings.Reserve(overallCfg.GetEntryCount());
    for (int i=0; i<overallCfg.GetEntryCount(); i++)
    {
      ParamEntryVal entry = overallCfg.GetEntry(i);
      if (entry.IsClass())
      {
        int idx = _overallSettings.Add();
        OverallQualityItem& val = _overallSettings[idx];
        val.LoadFromConfig(cfg, entry);
      }
    }
  }

  _benchmark.LoadFromConfig(cfg >> "Benchmark");
}

int VideoOptionsConfig::DetectCurrentOverallSettings(bool nearest)
{
  if (!GScene)
    return -1;

  if (!GEngine)
    return -1;

  // fill overall settings
  // WARNING - sometimes we fill value, sometimes index - depends on what we have available
  // this must reflect function VideoOptionsConfig::OverallQualityItem::GetDifferenceFrom

  OverallQualityItem currentSettings;
  currentSettings._visibility = GScene->GetPreferredViewDistance();
#if _HELISIM_VIEW_DIST
  currentSettings._objectsVisibility = GScene->GetPreferredObjectViewDistance();
#endif

#if _HELISIM_SHADOW_ZDISTANCE
  currentSettings._shadowsVisibility = GScene ? GScene->GetShadowZDistance() : Glob.config.GetMaxShadowSize();
#else
  currentSettings._shadowsVisibility = Glob.config.GetMaxShadowSize();
#endif
  currentSettings._hdrIndex = _hdr.FindNearest(Engine::HDRPrecToBits(GEngine->HDRPrecision()));
  currentSettings._ppaaIndex = GEngine->GetPPAAQuality();
  currentSettings._fsaaIndex = GEngine->GetFSAA();
  currentSettings._anisotropicFilteringIndex =_anisotropicFiltering.FindNearest(GEngine->GetAnisotropyQuality());

#if _HELISIM_RENDER_TO_TEXTURE
  currentSettings._pipIndex = GEngine->GetPipQuality();
#else
  currentSettings._pipIndex = 0;
#endif

  currentSettings._textureQualityIndex = GEngine->GetTextureQuality();
  currentSettings._objectQualityIndex = _objectQuality.FindNearest(GScene->GetComplexityTarget());
  currentSettings._terrainQualityIndex = _terrainQuality.FindNearest(GScene->GetPreferredTerrainGrid());

#if _ARMA3_SIMUL_WEATHER
  currentSettings._cloudQualityIndex = _cloudQuality.FindNearest(GEngine->GetSWGeometryLayersCount());
#endif

  currentSettings._shadowQualityIndex = GScene->GetShadowQuality();
  currentSettings._atocQuality = VideoOptionsConfig::ATOCValue::ConvertATOCMaskToValue((VideoOptionsConfig::ATOCValue::AToCMask)GEngine->GetAToC());

  // postprocesses
#if _ARMA3_SEPARATED_PP_SETTINGS
  currentSettings._ppSSAOQualityIndex = GEngine->GetSSAOQuality();
  currentSettings._ppCausticsQualityIndex = GEngine->GetCausticsQuality();
  currentSettings._ppBloom = GEngine->GetBloomScale();
  currentSettings._ppRotBlur = GEngine->GetRotBlurScale();
  currentSettings._ppRadialBlur = GEngine->GetRadialBlurScale();
  currentSettings._ppDOF = GEngine->GetDOFScale();
#else
  currentSettings._postProcessQuality = GEngine->GetPostprocessEffects();
#endif

  float bestDifference = FLT_MAX;
  int bestIndex = -1;
  for (int i=0; i<_overallSettings.Size(); ++i)
  {
    float difference = _overallSettings[i].GetDifferenceFrom(currentSettings);
    if (difference == 0.0f)
    {
      return i;
    }
    if (difference < bestDifference)
    {
      bestDifference = difference;
      bestIndex = i;
    }
  }

  if (nearest)
    return bestIndex;

  return -1;
}
