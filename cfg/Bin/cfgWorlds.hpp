//For fast path defining
#define PathDModel \core\default\default.p3d
#define PathDTexture \core\default_co.paa
#define PathDModelAbs core\default\default.p3d
#define PathDTextureAbs core\default_co.paa

#define ProcTextSkybox1 "#(argb,8,8,3)color(0.35,0.47,0.66,1)"
#define ProcTextSkybox2 "#(argb,8,8,3)color(0.46,0.51,0.67,1)"
#define ProcTextSkybox3 "#(argb,8,8,3)color(0.51,0.57,0.79,1)"
#define ProcTextSkybox4 "#(argb,8,8,3)color(0.56,0.6,0.76,1)"
#define ProcTextSkybox5 "#(argb,8,8,3)color(0.39,0.39,0.4,1)"

// define initial word state
class CfgWorlds
{
	access = ReadAndWrite;

  // new description of groups

  class GenericNames
  {
    access = ReadAndWrite;
    class Default // default naming scheme for non-persons (logics)
    {
      class FirstNames
      {
        default = "Unknown";
      };
      class LastNames
      {
        default = "Entity";
      };
    };
    #include "russiannames.hpp"
    #include "englishnames.hpp"
    #include "czechnames.hpp"
    #include "civiliannames.hpp"
  };

  #include "cfgLight.hpp"

	class DefaultWorld
	{
		access = ReadAndWrite;

    /// unique world ID used for Xbox 360 Matchmaking
    worldId = 0;

		plateFormat="$$$  ## - ##";
		plateLetters="ABCDEFHIKLMOPRSTVXYZ";

		//default center position
		centerPosition[] = {6400, 6400, 0};

		latitude = -40; // positive is south
		longitude = +15; // positive is east

		class Music: CfgMusic {};
		class EnvSounds: CfgEnvSounds {};

		cutscenes[] = {intro};

		/*Here you are able to replace objects placed in the world in Visitor by other models.
		This was used to replace generic forest models to the proper in-game models for example.
		You can either replace a model with a single other model or a set of other models.
		Example:
			class ForestTriangle
			{
				replace = PathDModelAbs;
				with[] = {};
			};
		center = true can be used as well to center all new objects.*/

		class ReplaceObjects {};

		landGrid = 50; // grid size
		// landRange is stored directly in WRP file

		gridNumbersOverLines = 0;	//displays Numbering over Grid lines instead between

		class Grid
		{
			offsetX = 0;
			offsetY = 0;

			class Zoom1
			{
				zoomMax = 0.1;
				format = "XY";
				formatX = "Aa";
				formatY = "00";
				stepX = 128;
				stepY = 128;
			};
			class Zoom2
			{
				zoomMax = 1e30;
				format = "XY";
				formatX = "A";
				formatY = "0";
				stepX = 1280;
				stepY = 1280;
			};
		};

		startTime = 16:45;
		startDate = 17/11/2006;
		startWeather = 0.12;
		startFog = 0.0;
    startWind = 0.1;
		forecastWeather = 0.3;
		forecastFog = 0.0;
		forecastWind = 0.1;

		class Lighting : DefaultLighting {};

    /** all DayLigthing values consist of 6 values:
      angle of the sun above the horizon
      max. direct light and min. direct light when not hidden by a cloud
      max. direct light and min. direct light when hidden by a cloud
      diffuse is sun or moon? (1=sun, 0=moon)
    */

    // offset to set a basic for luminance values
    #define LV (-4)

   class DayLightingBrightAlmost
    {
      // note: during the nighttime we want the moon to be used instead
      // moon only
      deepNight[]={
        -15, // sun angle
        {0.05,0.05,0.06},{0.001,0.001,0.002}, // max/min direct light
        {0.02,0.02,0.05},{0.003,0.003,0.003}, // max/min direct light (sun hidden by a cloud, the cloud is shinning)
        {0.003,0.003,0.006},{0.003,0.003,0.006}, // sky, sky around sun
        0 // sun - moon weighting
      };
      fullNight[]={
        -5, // sun angle
        {0.05,0.05,0.06},{0.02,0.02,0.02}, // max/min direct light
        {0.04,0.04,0.04},{0.04,0.04,0.04}, // max/min direct light (sun hidden by a cloud, the cloud is shinning)
        {0.01,0.01,0.02},{0.08,0.06,0.06}, // sky, sky around sun
        0 // sun - moon weighting
      };
      // moon-sun crossing - no diffuse lighting used, only ambient
      sunMoon[]={
        -3.75,
        {0.04,0.04,0.05},{0.04,0.04,0.05},
        {0.04,0.04,0.05},{0.04,0.04,0.05},
        {0.04,0.035,0.04},{0.11,0.08,0.09},
        0.5
      };
      // early sun - most light is in ambient, diffuse visible
      earlySun[]={
        -2.5,
        {0.32,0.24,0.28},{0.08,0.06,0.07},
        {0.32,0.24,0.28},{0.08,0.06,0.07},
        {0.08,0.07,0.08},{0.14,0.10,0.12},
        1
      };
      // sun only
      sunrise[]={
        0,
        {{0.7,0.45,0.45},5.5+LV},{{0.07,0.09,0.12},4.5+LV},
        {{0.6,0.47,0.25},4.7+LV}, {{0.10,0.09,0.10},4.4+LV},
        {{0.5,0.4,0.4},4.5+LV}, {{0.88,0.51,0.24},6+LV},
        1
      };
      earlyMorning[]={
        5,
	{{0.65,0.55,0.55},6+LV},{{0.08,0.09,0.11},5+LV},
	{{0.55,0.47,0.25},5.75+LV}, {{0.10,0.09,0.10},4.75+LV},
	{{0.5,0.4,0.4},7.5+LV}, {{0.88,0.51,0.24},8.75+LV},
        1
      };
      midMorning[]={
        15,
	{{0.98,0.85,0.8},10.5+LV},{{0.08,0.09,0.11},6.5+LV},
	{{0.87,0.47,0.25},9.5+LV}, {{0.09,0.09,0.10},7.5+LV},
	{{0.5,0.4,0.4},9.5+LV}, {{0.88,0.51,0.24},9.75+LV},
        1
      };
      morning[]={
	25,
	{{1,1,0.9},15.5+LV}, {{0.17,0.18,0.19},12.5+LV},
	{{1,1,0.9},14.5+LV}, {{0.17,0.18,0.19},13+LV},
	{{0.15,0.15,0.15},14.5+LV},{{0.17,0.17,0.15},15.5+LV},
        1
      };
      noon[]={
        70,
        {{0.96,0.98,1},15.75+LV},{{0.36,0.49,0.61},13+LV},
        {{1,1,1},15+LV},{{0.36,0.37,0.38},13.5+LV},
        {{0.9,0.9,0.9},15+LV},{{1.0,1.0,0.9},16+LV},
        1
      };
    };

    // class DayLightingCloudy

    class DayLightingRainy
    {
      // so many clouds there is little difference between cloud or clear
      deepNight[]={
        -15, // sun angle
        {0.0034,0.0034,0.004},{0.003,0.003,0.003}, // max/min direct light
        {0.0034,0.0034,0.004},{0.003,0.003,0.003}, // max/min direct light (sun hidden by a cloud, the cloud is shinning)
        {0.003,0.003,0.006},{0.003,0.003,0.006}, // sky, sky around sun
        0 // sun - moon weighting
      };
      fullNight[]={
        -5, // sun angle
        {0.023,0.023,0.023},{0.02,0.02,0.02}, // max/min direct light
        {0.023,0.023,0.023},{0.02,0.02,0.02}, // max/min direct light (sun hidden by a cloud, the cloud is shinning)
        {0.01,0.01,0.02},{0.08,0.06,0.06}, // sky, sky around sun
        0 // sun - moon weighting
      };

      // moon-sun crossing - no diffuse lighting used, only ambient
      sunMoon[]={
        -3.75,
        {0.04,0.04,0.05},{0.04,0.04,0.05},
        {0.04,0.04,0.05},{0.04,0.04,0.05},
        {0.04,0.035,0.04},{0.11,0.08,0.09},
        0.5
      };
      // early sun - most light is in ambient, diffuse visible
      earlySun[]={
        -2.5,
        {0.0689,0.0689,0.0804},{0.06,0.06,0.07},
        {0.0689,0.0689,0.0804},{0.06,0.06,0.07},
        {0.08,0.07,0.08},{0.14,0.10,0.12},
        0.5
      };
      // sun only
      #define RainlyLight {1,1,1}
      //#define RainlyLightColor {0.8,0.9,1}
      earlyMorning[]={
        0,
        {RainlyLight,LV+3.95},{RainlyLight,LV+3.75},
        {RainlyLight,LV+3.95},{RainlyLight,LV+3.75},
        {RainlyLight,LV+4},{RainlyLight,LV+4.5},
        1
      };
      morning[]={
        5,
        {RainlyLight,LV+5.7},{RainlyLight,LV+5.5},
        {RainlyLight,LV+5.7},{RainlyLight,LV+5.5},
        {RainlyLight,LV+7},{RainlyLight,LV+7.5},
        1
      };
      lateMorning[]={
        25,
        {RainlyLight,LV+10.45},{RainlyLight,LV+10.25},
        {RainlyLight,LV+10.45},{RainlyLight,LV+10.25},
        {RainlyLight,LV+12},{RainlyLight,LV+12.5},
        1
      };
      noon[]={
        70,
        {RainlyLight,LV+10.7},{RainlyLight,LV+10.5},
        {RainlyLight,LV+10.7},{RainlyLight,LV+10.5},
        {RainlyLight,LV+12.25},{RainlyLight,LV+12.75},
        1
      };
    };

		class Weather
		{
			godraysTexture = "core\data\godrays.paa";
			rainTexture = "core\data\raindrops_8888.paa";

			/// various parameters depending on "overcast" variable
			class Overcast
			{
				class Weather1
				{
					/// what overcast value is this item for
					overcast = 0;
					/// sky texture
					sky = ProcTextSkybox2;
          //! Sky reflection texture
          skyR = ProcTextSkybox2;
          //! Sky horizon texture
          horizon = ProcTextSkybox2;
					/// cloud alpha, brightness and speed
					alpha = 0;
					bright = 0.5;
					speed = 0.20;
					// relative size of the individual clouds
					size=0.2;
					// relative height of the clouds
					height=1.0;
					/// how much sun, moon and stars are visible through the sky box
					through = 1.0;
					// overcast value used for lighting
					lightingOvercast = 0;
					/// how much of the diffuse light will go through the sky box
					diffuse = 1.0;
					/// how much of the diffuse light will go through the cloud when occluded by it
					cloudDiffuse = 1.0;
					/// sea wave size
					waves = 0.15;
				};

				class Weather2
				{
					/// what overcast value is this item for
					overcast = 0.1;
					sky = ProcTextSkybox2;
          //! Sky reflection texture
          skyR = ProcTextSkybox2;
          //! Sky horizon texture
          horizon = ProcTextSkybox2;
					alpha = 0.3;
					size=0.3;
					height=0.9;
					bright = 0.5;
					speed = 0.25;
					through = 1.0;
          lightingOvercast = 0.1;
					diffuse = 1.0;
					cloudDiffuse = 0.9;
					waves = 0.22;
				};
				class Weather3
				{
					overcast = 0.4;
					sky = ProcTextSkybox3;
          //! Sky reflection texture
          skyR = ProcTextSkybox3;
          //! Sky horizon texture
          horizon = ProcTextSkybox2;
					alpha = 0.98;
					size=0.5;
					height=0.8;
					bright = 0.45;
					speed = 0.40;
					through = 0.8;

          lightingOvercast = 0.6;
					//class Lighting: DayLightingCloudy {};
					diffuse = 0.7;
					cloudDiffuse = 0.4;
					waves = 0.5;
				};
				class Weather4
				{
					overcast = 0.7;
					sky = ProcTextSkybox4;
          //! Sky reflection texture
          skyR = ProcTextSkybox4;
          //! Sky horizon texture
          horizon = ProcTextSkybox2;
					alpha = 0.97;
					size=0.75;
					height=0.75;
					bright = 0.45;
					speed = 0.7;
					through = 0.2;
					diffuse = 0.4;
          lightingOvercast = 0.9;
					// many clouds in the skybox - dynamic clouds do not change much
					cloudDiffuse = 0.75;
					waves = 0.6;
				};
				class Weather5
				{
					overcast = 1.0;
					sky = ProcTextSkybox5;
          //! Sky reflection texture
          skyR = ProcTextSkybox5;
          //! Sky horizon texture
          horizon = ProcTextSkybox2;
					alpha = 0.98;
					size=0.9;
					height=0.7;
					bright = 0.45;
					speed = 1.00;
					through = 0.0;
					diffuse = 0.2;
					// many clouds in the skybox - dynamic clouds do not change much
					cloudDiffuse = 0.95;
					waves = 0.7;
          lightingOvercast = 1.0;
				};
			};

			class Lighting
			{
			  class BrightAlmost: DayLightingBrightAlmost
			  {
			    overcast = 0;
			  };

			  class Rainy: DayLightingRainy
			  {
          overcast = 1.0;
			  };
			};
			class ThunderboltNorm
			{
				model = PathDModel;
				soundNear[] = {"", db-10, 1};
				soundFar[] = {"", db-10, 1};
			};
			class ThunderboltHeavy
			{
				model = PathDModel;
				soundNear[] = {"", db-10, 1};
				soundFar[] = {"", db-10, 1};
			};

      // Maximum and minimum temperatures (night and day) specified for every month - the game temperatures will vary in those limits // _VBS3_TI
      temperatureDayMax[]  ={ 10,  12,  15, 20, 25, 35, 35, 35, 25, 20,  10,  10};
      temperatureDayMin[]  ={-10, -6, -5, -1,  5,  6,  7, 10,  5,  2, -5, -10};

      temperatureNightMax[]={  5,   6,   8, 10, 13, 18, 26, 25, 15, 13,   8,   4};
      temperatureNightMin[]={-10, -10, -10, -5,  0,  4,  5,  6,  5,  0,  -5, -10};

      // How much we limit the minimum (resp. maximum) temperature during the day (resp. night) if the sky is clear  // _VBS3_TI
      // 0 means no limit (the max and min will stay as stated above), 1 means full limit (the min and max will be equal)
      overcastTemperatureFactor = 0.4;

      // How much can be black and white surfaces heated up on a direct sunlight  // _VBS3_TI
      blackSurfaceTemperatureDelta = +5;
      whiteSurfaceTemperatureDelta = -2;
		};
		class Rain
		{
			texture = PathDTextureAbs;
			speed = 1;
			levels[]={8, 2};
			raindrop = PathDModel;
		};

    class SimulWeather
    {
      // Texture used for cloud noise
      // Use _raw for uncompressed paa or uncompressed dds file (compression caused artefacts)
      noiseTexture = "core\data\noise_raw.paa";

      //=========================================================
      // General values
      //=========================================================
      // Number of keyframes per day. More keyframes = slower game (because of tables generation for each keyframe)
      numKeyframesPerDay = 48;  //48 = keyframe every 30 minutes
      // multiplier for wind speed
      windSpeedCoef = 10.0f;
      // coef that is used to multiply moon irradiance
      moonIrradianceCoef = 10.0f;

      //=========================================================
      // Values used for fade and sky color calculations
      //=========================================================
      // The maximum distance for the fade
      fadeMaxDistanceKm = 300.0;        
      // The maximum altitude that will be used for fade and sky colour calculations. Few aircraft can fly higher than 15km
      fadeMaxAltitudeKm = 15.0;
      // How many altitudes to calculate fade and sky colours for - minimum 1, maximum 64.
      fadeNumAltitudes = 4;      
      // How many elevation steps are used to calculate fade and sky colours (height of the fade table)
      fadeNumElevations = 8;      
      // How many distance steps are used to calculate fade and sky colours (width of the fade table)
      fadeNumDistances = 8; 
      // Whether light should stop at the planet's surface - normally false
      fadeEarthTest = false;

      //=========================================================
      // Values for sky keyframer
      //=========================================================
      // Whether the brightness of each keyframe is calculated automatically based on the available, sun, moon, or starlight.
      autoBrightness = true;
      // For AutoBrightness, how strongly the brightness is adjusted for darker skies
      autoBrightnessStrength = 0.1;

      //=========================================================
      // Values for cloud grid
      //=========================================================
      // Width of the cloud grid
      cloudGridWidth = 128;
      // Length of the cloud grid
      cloudGridLength = 128;
      // Height of the cloud grid
      cloudGridHeight = 16;

      //=========================================================
      // Values for Helper for generating cloud geometry
      //=========================================================
      // Helper elevation grid size
      helperGridElevationSteps = 24;
      // Helper azimuth grid size
      helperGridAzimuthSteps = 15;
      //! Set the effective radius of the Earth in metres, for curving the cloud volumes. The actual radius is ~6378000 metres, but
      //! smaller values produce more curvature. If set to zero, the radius will be calculated to make the clouds go to the horizon
      //! at the maximum cloud distance.
      helperEffectiveEarthRadius = 6378000;
      // Specify whether to curve the cloud volume over the Earth's surface, based on the effective radius.
      helperCurvedEarth = true;
      // Not sure about this...
      helperAdjustCurvature = false;
      // Number of layers used for geometry rendering (more layers = lower fps, values should be between 12-120)
      helperNumLayers = 64;
      // Distance where the furthest rendered layer is placed 
      helperMaxDistance = 200000;
      //! Set a value, usually between 0 and 1, that determines how close cloud layers must be to the camera before starting to fade out.
      //! This value is a multiple for the distance between cloud layers at the nearest point.
      helperNearCloudFade = 0.5;
      //! Set the texture churn - how much the texture churns per frame - zero gives a static appearance,
      //! values up to 1 and over make the texture animate.
      helperChurn = 10;

      //=========================================================
      // Values for cloud generation
      //=========================================================
      // Width (East-West) of the whole area covered with clouds (in metres)
      cloudWidth = 60000;
      // Length (North-South) of the whole area covered with clouds (in metres)
      cloudLength = 60000;
      // If wrapping of clouds is enabled
      wrapClouds = true;

      // Resolution of the 3D Perlin noise used to generate clouds
      noiseResolution = 8;
      // Period (in daytime units) over which noise changes
      noisePeriod = 4.0;

      // density of clouds 
      opticalDensity = 1.8;

      // How sharply defined the cloud edges are (0-1 (1=sharpest))
      alphaSharpness = 0.15;
      // Scale of self-shadowing, zero=complete,one=none
      selfShadowScale = 0.001;
      // Asymmetry parameter, between -1 and 1, for scattering. For clouds, values between 0.5 and 0.9 are most common.
      mieAsymmetry = 0.87;
      // Minimum elevation for lighting - if light direction is below this angle, it will be raised - useful for sunsets
      minimumLightElevationDegrees = 3.0;

      // global coef, that is used to multiply direct lighting of clouds
      directLightCoef = 0.25;
      // global coef, that is used to multiply indirect lighting of clouds
      indirectLightCoef = 0.01;

      // start of the Arma fog for clouds (im meters)
      fogStart = 200000;
      // end of the Arma fog for clouds (in meters)
      fogEnd = 300000;

      // default keyframe values - will be overriden by keyframe values set in Overcast class
      class DefaultKeyframe
      {
        // Sequencer file name (use this to load values from specific keyframe within sequencer file)
        // seqFile = "";
        // Number of keyframe in sequencer file (beginning with 0) (use this to load values from specific keyframe within sequencer file)
        // seqFileKeyframe = 0;     

        // =========================
        // Sky keyframe 
        // =========================
        // The Rayleigh scattering coefficients (x=red,y=green,z=blue).
        rayleigh[] = {0.00749, 0.01388, 0.02878};
        // The Mie scattering coefficients (x=red,y=green,z=blue).
        mie[] ={0.0046, 0.0046, 0.0046};

        // The amount of haze, mist or fog.
        haze = 1.0;
        // The base altitude, above which haze starts to decrease in density.
        hazeBaseKm = 1.0;
        // The vertical scale over which haze reduces with altitude.
        hazeScaleKm = 0.5;
        // The anisotropy of Mie scattering.
        hazeEccentricity = 0.82;
        // Brightness adjustment of the keyframe - this value is used only if autoBrightness is disabled
        brightnessAdjustment = 1.0;

        // =========================
        // Cloud keyframe 
        // =========================
        // The density of the cloud layer at this time (0-1)
        cloudiness = 0.5;
        // The base altitude of this cloud layer.
        cloudBaseKm = 3.2;
        // The height of the cloud layer, above its base altitude.
        cloudHeightKm = 6;

        // The amount of direct light to be used for rendering.
        directLight = 1.0;
        // The amount of indirect or secondary light to be used for rendering.
        indirectLight = 1.0;
        // The amount of ambient light to be used for rendering
        ambientLight = 1.0;

        // The number of noise octaves used to generate the clouds; default 3.
        noiseOctaves = 3;
        // The fractal persistence for generating the clouds, default 0.45.
        noisePersistence = 0.55;
        // The strength of the fractal edge effect, default 1.8.
        fractalAmplitude = 2.2;
        //  The wavelength of the fractal edge effect - longer wavelengths produce smoother clouds, default 140.
        fractalWavelength = 220.0;

        // The amount of light scattered per unit distance - larger values produce darker clouds, default 1.7.
        extinction = 1.8;
        // How much the edges of clouds should be diffused.
        diffusivity = 0.01;
      };

      class Overcast
      {
        class Weather1 : DefaultKeyframe
        {
          overcast = 0.0;
          cloudiness = 0.0;
        };
        class Weather2 : DefaultKeyframe
        {
          overcast = 0.2;
          cloudiness = 0.1;
        };
        class Weather3 : DefaultKeyframe
        {
          overcast = 0.4;
          cloudiness = 0.2;
        };
        class Weather4 : DefaultKeyframe
        {
          overcast = 0.6;
          cloudiness = 0.4;
        };
        class Weather5 : DefaultKeyframe
        {
          overcast = 0.8;
          cloudiness = 0.5;
        };
        class Weather6 : DefaultKeyframe
        {
          overcast = 1.0;
          cloudiness = 0.6;
        };
      };
    };

    /// sky texture, used only as default when there is no weather
    skyTexture = ProcTextSkybox1;
    //! Sky reflection texture, used only as default when there is no weather
    skyTextureR = ProcTextSkybox1;
    /// texture used for water surface
    // wave detection texture expected by the shaders
    seaTexture = "#(rgb,8,8,3)color(0,0,0,1)";

    midDetailTexture = "#(rgb,1,1,1)color(0.5,0.5,0.5,1,cdt)";
    /// texture used outside of the map (sea bed or infinite desert)
    //outsideMaterial = PathToLandText\pt.bimpas;
    outsideMaterial ="";
    /// height used outside of the map
    outsideHeight = -10;
    /// min. height - used to clamp data inside of the map during rendering
    minHeight = -10;
    /// shore effects - based on water depth (negative means water above the water level)
    shoreTop = 0; // top point where water is considered off-shore
    peakWaveTop = -1; // point where water peaks should be max. visible before going offshore
    peakWaveBottom = -5; // point where water peaks should start appearing


    clouds[] = {PathDModelAbs, PathDModelAbs, PathDModelAbs, PathDModelAbs};

    /// where ground detail texture is no longer visible (end fading out)
    noDetailDist = 50;
    /// where ground detail texture is fully visible (begin fading out)
    fullDetailDist = 20;


    /// distance, where ground detail normal is starts to blend to satellite normal texture
    satelliteNormalBlendStart = 80;
    /// distance, where blending of detail normal ends (satellite normal is fully visible)
    satelliteNormalBlendEnd = 100;

    /// values for new haze computation (see _ARMA3_HAZE_COMPUTATION)
    /// coefficient used to compute haze distance (haze distance = fogFar * hazeDistCoef), can be > 1
    /// if the value is negative, then standard haze computation is used
    hazeDistCoef = -1.0;
    /// fog value, that we want to have at haze distance (=distance computed using hazeDistCoef), 0 = full fog, 1 = no fog
    /// if the value is negative, then standard haze computation is used
    hazeFogCoef = -1.0;

    /// controls density of clutters (size of square where single clutter is present)
    clutterGrid = 2;
    /// how far clutters are visible
    clutterDist = 50;
    /// maximal size of clutters (for clipping)
    clutterRadius = 1.0;
    class DefaultClutter
    {
      /// Determines the affection of the clutter to the wind (0 - stone, 1 - common grass)
      affectedByWind = 0;
      /// how much is the clutter colored by the ground
      //colorByGround = 0.5; //obsolete
      /// if colored by the ground, desired color relative to the ground
      relativeColor[] = {1, 1, 1, 1};
      /// minimal scale
      scaleMin = 0.5;
      /// maximal scale
      scaleMax = 1.0;
      /// Use default lighting
      swLighting = true;
    };
    class Clutter {};

    skyObject = "core\skyobject\skyobject.p3d";
    starsObject = PathDModelAbs;
    pointObject = PathDModelAbs;
    horizontObject = PathDModelAbs;
    haloObject = PathDModelAbs;
    sunObject = PathDModelAbs;
    rainbowObject = PathDModelAbs;
    moonObject = PathDModelAbs;
    waterTexture = ProcTextSkybox1;

    //============================================
    // env texture
    //============================================
    /// no env texture by default
    envTexture = "";

    //@{ layered terrain rendering data
    satelliteMap = "";
    layerMask = "";
    //@}

    // Water material is used for sea landscape parts
    seaMaterial = "#water";

    // Shore material is used for sea and landscape interface parts
    shoreMaterial = "#shore";

    // Shore material is used for sea and landscape interface parts - the foam part
    shoreFoamMaterial = "#shoreFoam";

    // Shore wet material is used for sea and landscape interface parts - the wet part
    shoreWetMaterial = "#shoreWet";

    // Terrain material is used for terrain solid ground
    // by default load from CfgMaterials
    terrainMaterial = "#terrain";

		// fractal and white noise random offset are added
		// white noise generated more "wild" terrain
		// fractal tends to generate smooth, varied terrain
		class Subdivision
		{
			// fractal component of subdivision
			// changes are smaller for smaller rectangles
			class Fractal
			{
				// texture roughness factor
				rougness = 10;
				// max. value for squares containing road
				maxRoad = 0.2;
				// max. value for squares containing track
				maxTrack = 1.0;
				// max. coeficient depending on slope
				maxSlopeFactor = 0.05;
			};
			// white noise component of subdivision
			// change size is independent on rectangle size
			class WhiteNoise
			{
				rougness = 5;
				// max. value for squares containing road
				maxRoad = 0.1;
				// max. value for squares containing track
				maxTrack = 0.5;
				// max. coeficient depending on slope
				maxSlopeFactor = 0.025;
			};

			// do not divide surfaces that are under given limit
			minY = -0.0;
			// do not divide flat surfaces
			minSlope = 0.02;
		};

		/// ambient life configuration
		class Ambient
		{
		  /*
		  class Birds
		  {
		    /// container radius
		    radius=170;
		    /// points allowed for this container
		    cost=3;
  		  /// classname in Species corresponds to type name
		    class Species
		    {
		      class Seagull
		      {
		        probability=1;
		        cost=1;
		      };
		    };
		    class Insects
		    {
		      radius=3;
		      cost=5;
		      /// species configuration
		      class Species
		      {
		        class Fly
		        {
		          probability=0.9;
		          cost=1;
		        };
		        class Mosquito
		        {
		          probability=0.1;
		          cost=1;
		        };
		      };
		    };
		  };
		  */
		};
	};


	initWorld = "";
	demoWorld = "";


};

class CfgWorldList
{
	// allow register new worlds here
	access = ReadAndCreate;
};
#undef PathDModel
#undef PathDModelAbs
#undef PathDTexture
#undef PathDTextureAbs