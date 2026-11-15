// TODO LOCALIZE
#define TEXT_DISABLED "$STR_DISABLED"
#define TEXT_ENABLED "$STR_ENABLED"
#define TEXT_VERY_LOW "$STR_TERRAIN_50"
#define TEXT_LOW "$STR_TERRAIN_25"
#define TEXT_NORMAL "$STR_TERRAIN_12_5"
#define TEXT_HIGH "$STR_TERRAIN_6_25"
#define TEXT_VERY_HIGH "$STR_TERRAIN_3_125"

class CfgVideoOptions
{
  // overall visibility 
  class Visibility
  {
    minValue = 500;
    maxValue = 12000;
  };

  // visibility of objects
  class ObjectsVisibility
  {
    minValue = 500;
    // note - max value is always limited by current overall visibility value
    maxValue = 12000;
  };

  // visibility of shadows
  class ShadowsVisibility
  {
    minValue = 50;
    maxValue = 200;
  };

  // HDR quality
  class HDR
  {
    class Low
    {
      text = TEXT_VERY_LOW;
      value = 8; // 8 bits per pixel
    };
    class Normal
    {
      text = TEXT_LOW;
      value = 16; // 16 bits per pixel
    };
  };

  // quality of post process antialiasing
  class PPAA
  {
    class Disabled
    {
      textMethod = "";
      textQuality = TEXT_DISABLED;
      // type of ppaa (0 = disabled, 1 = FXAA, 2 = SMAA)
      ppaaType = 0;
      // quality level of ppaa (0 = worst)      
      ppaaLevel = 0;
    };

    class FXAALow
    {
      textMethod = "FXAA";
      textQuality = TEXT_LOW;
      // type of ppaa (0 = disabled, 1 = FXAA, 2 = SMAA)
      ppaaType = 1;
      // quality level of ppaa (0 = worst)      
      ppaaLevel = 0;
    };
    class FXAANormal
    {
      textMethod = "FXAA";
      textQuality = TEXT_NORMAL;
      // type of ppaa (0 = disabled, 1 = FXAA, 2 = SMAA)
      ppaaType = 1;
      // quality level of ppaa (0 = worst)      
      ppaaLevel = 1;
    };
    class FXAAHigh
    {
      textMethod = "FXAA";
      textQuality = TEXT_HIGH;
      // type of ppaa (0 = disabled, 1 = FXAA, 2 = SMAA)
      ppaaType = 1;
      // quality level of ppaa (0 = worst)      
      ppaaLevel = 2;
    };
    class FXAAVeryHigh
    {
      textMethod = "FXAA";
      textQuality = TEXT_VERY_HIGH;
      // type of ppaa (0 = disabled, 1 = FXAA, 2 = SMAA)
      ppaaType = 1;
      // quality level of ppaa (0 = worst)      
      ppaaLevel = 3;
    };
    class SMAALow
    {
      textMethod = "SMAA";
      textQuality = TEXT_LOW;
      // type of ppaa (0 = disabled, 1 = FXAA, 2 = SMAA)
      ppaaType = 2;
      // quality level of ppaa (0 = worst)      
      ppaaLevel = 0;
    };
    class SMAANormal
    {
      textMethod = "SMAA";
      textQuality = TEXT_NORMAL;
      // type of ppaa (0 = disabled, 1 = FXAA, 2 = SMAA)
      ppaaType = 2;
      // quality level of ppaa (0 = worst)      
      ppaaLevel = 1;
    };
    class SMAAHigh
    {
      textMethod = "SMAA";
      textQuality = TEXT_HIGH;
      // type of ppaa (0 = disabled, 1 = FXAA, 2 = SMAA)
      ppaaType = 2;
      // quality level of ppaa (0 = worst)      
      ppaaLevel = 2;
    };
    class SMAAVeryHigh
    {
      textMethod = "SMAA";
      textQuality = TEXT_VERY_HIGH;
      // type of ppaa (0 = disabled, 1 = FXAA, 2 = SMAA)
      ppaaType = 2;
      // quality level of ppaa (0 = worst)      
      ppaaLevel = 3;
    };

  };

  class AnisotropicFiltering
  {
    class Disabled
    {
      text = TEXT_DISABLED;
      //max anisotropy
      value = 0; 
    };
    class VeryLow
    {
      text = TEXT_VERY_LOW;
      //max anisotropy
      value = 2; 
    };
    class Low
    {
      text = TEXT_LOW;
      //max anisotropy
      value = 4; 
    };
    class Normal
    {
      text = TEXT_NORMAL;
      //max anisotropy
      value = 8; 
    };
    class High
    {
      text = TEXT_HIGH;
      //max anisotropy
      value = 12; 
    };
    class VeryHigh
    {
      text = TEXT_VERY_HIGH;
      //max anisotropy
      value = 16; 
    };
  };

  class PiP
  {
    class Disabled
    {
      text = TEXT_DISABLED;
      /// max visibility for PiP cameras (0 = disabled)
      value = 0;
    };
    class VeryLow
    {
      text = TEXT_VERY_LOW;
      /// max visibility for PiP cameras
      value = 500;
    };
    class Low
    {
      text = TEXT_LOW;
      /// max visibility for PiP cameras
      value = 800;
    };
    class Normal
    {
      text = TEXT_NORMAL;
      /// max visibility for PiP cameras
      value = 1000;
    };
    class High
    {
      text = TEXT_HIGH;
      /// max visibility for PiP cameras
      value = 1500;
    };
    class VeryHigh
    {
      text = TEXT_VERY_HIGH;
      /// max visibility for PiP cameras
      value = 2000;
    };
  };

  class TextureQuality
  {
    class VeryLow
    {
      text = TEXT_VERY_LOW;
      /// mipmap bias for textures
      mipBias = 5.0;
    };
    class Low
    {
      text = TEXT_LOW;
      /// mipmap bias for textures
      mipBias = 3.7;
    };
    class Normal
    {
      text = TEXT_NORMAL;
      /// mipmap bias for textures
      mipBias = 2.0;
    };
    class High
    {
      text = TEXT_HIGH;
      /// mipmap bias for textures
      mipBias = 1.5;
    };
    class VeryHigh
    {
      text = TEXT_VERY_HIGH;
      /// mipmap bias for textures
      mipBias = 1.0;
    };
  };

  class ObjectsQuality
  {
    class VeryLow
    {
      text = TEXT_VERY_LOW;
      /// complexity target of the scene (target number of polygons in the scene)
      value = 200000;
    };
    class Low
    {
      text = TEXT_LOW;
      /// complexity target of the scene (target number of polygons in the scene)
      value = 300000;
    };
    class Normal
    {
      text = TEXT_NORMAL;
      /// complexity target of the scene (target number of polygons in the scene)
      value = 500000;
    };
    class High
    {
      text = TEXT_HIGH;
      /// complexity target of the scene (target number of polygons in the scene)
      value = 750000;
    };
    class VeryHigh
    {
      text = TEXT_VERY_HIGH;
      /// complexity target of the scene (target number of polygons in the scene)
      value = 1000000;
    };
  };

  class TerrainQuality
  {
    class VeryLow
    {
      text = TEXT_VERY_LOW;
      // terrain grid (larger value = worse quality)
      // DO NOT change this value, engine is dependent on it
      terrainGrid = 50.0;
    };
    class Low
    {
      text = TEXT_LOW;
      // terrain grid (larger value = worse quality)
      // DO NOT change this value, engine is dependent on it
      terrainGrid = 25.0;
    };
    class Normal
    {
      text = TEXT_NORMAL;
      // terrain grid (larger value = worse quality)
      // DO NOT change this value, engine is dependent on it
      terrainGrid = 12.5f;
    };
    class High
    {
      text = TEXT_HIGH;
      // terrain grid (larger value = worse quality)
      // DO NOT change this value, engine is dependent on it
      terrainGrid = 6.25f;
    };
    class VeryHigh
    {
      text = TEXT_VERY_HIGH;
      // terrain grid (larger value = worse quality)
      // DO NOT change this value, engine is dependent on it
      terrainGrid = 3.125f;
    };
  };

  class CloudQuality
  {
    class Disabled
    {
      text = TEXT_DISABLED;
      // number of Simul Weather layers (0 = Simul Weather disabled)
      value = 0;
    };
    class VeryLow
    {
      text = TEXT_VERY_LOW;
      // number of Simul Weather layers (0 = Simul Weather disabled)
      value = 32;
    };
    class Low
    {
      text = TEXT_LOW;
      // number of Simul Weather layers (0 = Simul Weather disabled)
      value = 48;
    };
    class Normal
    {
      text = TEXT_NORMAL;
      // number of Simul Weather layers (0 = Simul Weather disabled)
      value = 64;
    };
    class High
    {
      text = TEXT_HIGH;
      // number of Simul Weather layers (0 = Simul Weather disabled)
      value = 96;
    };
    class VeryHigh
    {
      text = TEXT_VERY_HIGH;
      // number of Simul Weather layers (0 = Simul Weather disabled)
      value = 128;
    };
  };

  class ShadowQuality
  {
    class Disabled
    {
      text = TEXT_DISABLED;
      // type of shadows (0 = disabled, 1 = stencil, 2 = shadow buffer)
      shadowType = 0;
      // size of texture for shadow buffer shadows
      textureSize = 0;
      // quality of shader for shadow buffer shadows (0=1 PCF tap, 1=4PCF taps, 2=4-16 PCF taps, 3=4-16 PCF taps + alpha blend)
      shaderQuality = 0;
    };
    class VeryLow
    {
      text = TEXT_VERY_LOW;
      // type of shadows (0 = disabled, 1 = stencil, 2 = shadow buffer)
      shadowType = 1;
      // size of texture for shadow buffer shadows
      textureSize = 0;
      // quality of shader for shadow buffer shadows (0=1 PCF tap, 1=4PCF taps, 2=4-16 PCF taps, 3=4-16 PCF taps + alpha blend)
      shaderQuality = 0;
    };
    class Low
    {
      text = TEXT_LOW;
      // type of shadows (0 = disabled, 1 = stencil, 2 = shadow buffer)
      shadowType = 2;
      // size of texture for shadow buffer shadows
      textureSize = 1024;
      // quality of shader for shadow buffer shadows (0=1 PCF tap, 1=4PCF taps, 2=4-16 PCF taps, 3=4-16 PCF taps + alpha blend)
      shaderQuality = 1;
    };
    class Normal
    {
      text = TEXT_NORMAL;
      // type of shadows (0 = disabled, 1 = stencil, 2 = shadow buffer)
      shadowType = 2;
      // size of texture for shadow buffer shadows
      textureSize = 1024;
      // quality of shader for shadow buffer shadows (0=1 PCF tap, 1=4PCF taps, 2=4-16 PCF taps, 3=4-16 PCF taps + alpha blend)
      shaderQuality = 2;
    };
    class High
    {
      text = TEXT_HIGH;
      // type of shadows (0 = disabled, 1 = stencil, 2 = shadow buffer)
      shadowType = 2;
      // size of texture for shadow buffer shadows
      textureSize = 2048;
      // quality of shader for shadow buffer shadows (0=1 PCF tap, 1=4PCF taps, 2=4-16 PCF taps, 3=4-16 PCF taps + alpha blend)
      shaderQuality = 2;
    };
    class VeryHigh
    {
      text = TEXT_VERY_HIGH;
      // type of shadows (0 = disabled, 1 = stencil, 2 = shadow buffer)
      shadowType = 2;
      // size of texture for shadow buffer shadows
      textureSize = 2048;
      // quality of shader for shadow buffer shadows (0=1 PCF tap, 1=4PCF taps, 2=4-16 PCF taps, 3=4-16 PCF taps + alpha blend)
      shaderQuality = 3;
    };
  };

  class ATOCQuality
  {
    class Disabled
    {
      text = TEXT_DISABLED;
      
      // ATOC for grass
      grass = false;
      // ATOC for old tree shader
      oldTrees = false;
      // ATOC for new tree shader
      newTrees = false;
      // ATOC for other shaders (SuperHair, SuperATOC)
      other = false;
    };

    class Grass: Disabled
    {
      text = $STR_ATOC_GRASS; //TODO LOCALIZE
      // ATOC for grass
      grass = true;
      // ATOC for other shaders (SuperHair, SuperATOC)
      // always enable others when something is enabled
      other = true;
    };

    class OldTree: Disabled
    {
      text = $STR_ATOC_TREEA2; //TODO LOCALIZE
      // ATOC for old tree shader
      oldTrees = true;
      // ATOC for other shaders (SuperHair, SuperATOC)
      // always enable others when something is enabled
      other = true;
    };

    class OldTreeGrass: Disabled
    {
      text = $STR_ATOC_TREEA2_GRASS; //TODO LOCALIZE
      // ATOC for grass
      grass = true;
      // ATOC for old tree shader
      oldTrees = true;
      // ATOC for other shaders (SuperHair, SuperATOC)
      // always enable others when something is enabled
      other = true;
    };

    class NewTree: Disabled
    {
      
      text = $STR_ATOC_TREEOA; //TODO LOCALIZE
      // ATOC for new tree shader
      newTrees = true;
      // ATOC for other shaders (SuperHair, SuperATOC)
      // always enable others when something is enabled
      other = true;
    };

    class NewTreeGrass: Disabled
    {
      
      text = $STR_ATOC_TREEOA_GRASS; //TODO LOCALIZE
      // ATOC for grass
      grass = true;
      // ATOC for new tree shader
      newTrees = true;
      // ATOC for other shaders (SuperHair, SuperATOC)
      // always enable others when something is enabled
      other = true;
    };

    class OldTreeNewTree: Disabled
    {
      text = $STR_ATOC_TREEOA_TREEA2; //TODO LOCALIZE
      // ATOC for old tree shader
      oldTrees = true;
      // ATOC for new tree shader
      newTrees = true;
      // ATOC for other shaders (SuperHair, SuperATOC)
      // always enable others when something is enabled
      other = true;
    };

    class All: Disabled
    {
      text = $STR_ATOC_TREEOA_TREEA2_GRASS; //TODO LOCALIZE
      // ATOC for grass
      grass = true;
      // ATOC for old tree shader
      oldTrees = true;
      // ATOC for new tree shader
      newTrees = true;
      // ATOC for other shaders (SuperHair, SuperATOC)
      // always enable others when something is enabled
      other = true;
    };
  };

  class FSAA
  {
    class Disabled
    {
      text = TEXT_DISABLED;
      value = 0;
    };
    class Low
    {
      text = TEXT_LOW;
      value = 1;
    };
    class Normal
    {
      text = TEXT_NORMAL;
      value = 2;
    };
    class High
    {
      text = TEXT_HIGH;
      value = 3;
    };
    class VeryHigh
    {
      text = TEXT_VERY_HIGH;
      value = 4;
    };
  };

  class PPBloom
  {
    minValue = 0;
    maxValue = 2;
  };

  class PPDOF
  {
    minValue = 0;
    maxValue = 2;
  };

  class PPRotBlur
  {
    minValue = 0;
    maxValue = 2;
  };

  class PPRadialBlur
  {
    minValue = 0;
    maxValue = 2;
  };

  class DefaultSettings
  {
    text = "Default";

    visibility = 1600;
    objectVisibility = 1200;
    shadowVisibility = 100;
    hdr = Normal;
    ppaa = Disabled;
    fsaa = Disabled;
    particles = Normal;
    dynamicLights = Normal;
    anisotropicFiltering = Normal;
    pip = Normal;
    textureQuality = Normal;
    objectsQuality = Normal;
    terrainQuality = Normal;
    cloudQuality = Normal;
    shadowQuality = Normal;
    atocQuality = All;

    ppBloom = 1;
    ppDOF = 1;
    ppRotBlur = 1;
    ppRadialBlur = 1;

    // postprocess quality (used when separate PP settings are not used)
    postProcessQuality = 3;
  };

  class OverallSettings
  {
    class VeryLow: DefaultSettings
    {
      text = TEXT_VERY_LOW;

      visibility = 1000;
      objectVisibility = 800;
      shadowVisibility = 100;
      hdr = Normal;
      fsaa = Disabled;
      ppaa = Disabled;
      particles = Low;
      dynamicLights = VeryLow;
      anisotropicFiltering = Disabled;
      pip = VeryLow;
      textureQuality = VeryLow;
      objectsQuality = VeryLow;
      terrainQuality = Low; //we never want terrain to be VeryLow (=no grass) when setting overall quality 
      cloudQuality = VeryLow;
      shadowQuality = Low;
      atocQuality = Disabled;

      ppBloom = 0;
      ppDOF = 0;
      ppRotBlur = 0;
      ppRadialBlur = 0;

      // postprocess quality (used when separate PP settings are not used)
      postProcessQuality = 0;
    };

    class Low: DefaultSettings
    {
      text = TEXT_LOW;

      visibility = 1600;
      objectVisibility = 1300;
      shadowVisibility = 100;
      hdr = Normal;
      fsaa = Disabled;
      ppaa = Disabled;
      particles = Low;
      dynamicLights = Low;
      anisotropicFiltering = Low;
      pip = Low;
      textureQuality = Low;
      objectsQuality = Low;
      terrainQuality = Low;
      cloudQuality = Low;
      shadowQuality = Normal;
      atocQuality = Disabled;

      ppBloom = 1;
      ppDOF = 1;
      ppRotBlur = 1;
      ppRadialBlur = 1;
 
      // postprocess quality (used when separate PP settings are not used)
      postProcessQuality = 2;
    };

    class Normal: DefaultSettings
    {
      text = TEXT_NORMAL;

      visibility = 2200;
      objectVisibility = 1800;
      shadowVisibility = 100;
      hdr = Normal;
      fsaa = Low;
      ppaa = FXAALow;
      particles = Normal;
      dynamicLights = Normal;
      anisotropicFiltering = Normal;
      pip = Normal;
      textureQuality = Normal;
      objectsQuality = Normal;
      terrainQuality = Normal;
      cloudQuality = Normal;
      shadowQuality = Normal;
      atocQuality = All;

      ppBloom = 1;
      ppDOF = 1;
      ppRotBlur = 1;
      ppRadialBlur = 1;

      // postprocess quality (used when separate PP settings are not used)
      postProcessQuality = 3;
    };

    class High: DefaultSettings
    {
      text = TEXT_HIGH;

      visibility = 3000;
      objectVisibility = 2600;
      shadowVisibility = 100;
      hdr = Normal;
      fsaa = Normal;
      ppaa = FXAANormal;
      particles = High;
      dynamicLights = High;
      anisotropicFiltering = High;
      pip = Normal;
      textureQuality = High;
      objectsQuality = Normal; // avoid using some settings as High for High by default, use it for Very High only
      terrainQuality = Normal;
      cloudQuality = High;
      shadowQuality = High;
      atocQuality = All;

      ppBloom = 1;
      ppDOF = 1;
      ppRotBlur = 1;
      ppRadialBlur = 1;

      // postprocess quality (used when separate PP settings are not used)
      postProcessQuality = 4;
    };

    class VeryHigh: DefaultSettings
    {
      text = TEXT_VERY_HIGH;

      visibility = 3800;
      objectVisibility = 3200;
      shadowVisibility = 100;
      hdr = Normal;
      fsaa = VeryHigh;
      ppaa = FXAAVeryHigh;
      particles = High;
      dynamicLights = VeryHigh;
      anisotropicFiltering = VeryHigh;
      pip = Normal;
      textureQuality = VeryHigh;
      objectsQuality = High; // avoid using some settings as High for High by default, use it for Very High only
      terrainQuality = VeryHigh;
      cloudQuality = VeryHigh;
      shadowQuality = VeryHigh;
      atocQuality = All;

      ppBloom = 1;
      ppDOF = 1;
      ppRotBlur = 1;
      ppRadialBlur = 1;

      // postprocess quality (used when separate PP settings are not used)
      postProcessQuality = 5;
    };
  };

  // benchmark configuration
  class Benchmark
  {
    // default result is returned when benchmark fails or is disabled
    // this is a time in ms (duration of benchmark)
    defaultResult = 1000;

    // This determines how to obtain overall setting based on benchmark result.
    // Values are times in ms (benchmark avg duration).
    // if result > firstValue, overallQuality = VeryLow
    // else if result > secondValue, overallQuality = Low
    // etc...
    // Values were obtained from benchmark result on various GPUs (best GPUs are around 60ms, mediocre around 130ms)
    resultLimits[] = {300, 150, 120, 90, 0};
  };
};
