// Configuration parameters for ArmA2Free
// - public SP & MP demo

#define _VERIFY_KEY								1
#define _VERIFY_CLIENT_KEY  			1  // server is checking the CD key of clients on GameSpy
#define _VERIFY_BLACKLIST					0
#define _ENABLE_EDITOR						1
#define _ENABLE_WIZARD						1
#define _ENABLE_ALL_MISSIONS			0
#define _ENABLE_MP								0
#define _ENABLE_AI								1
#define _ENABLE_CAMPAIGN					0
#define _ENABLE_SINGLE_MISSION		1
#define _ENABLE_UNSIGNED_MISSIONS	1
#define _ENABLE_BRIEF_GRP					1
#define _ENABLE_PARAMS						1
#define _ENABLE_CHEATS						0
#define _ENABLE_ADDONS						1
#define _ENABLE_DEDICATED_SERVER	0
#define _ENABLE_HOST              0
#define _FORCE_DEMO_ISLAND				0
#define _FORCE_SINGLE_VOICE				0
#define _ENABLE_AIRPLANES					1
#define _ENABLE_HELICOPTERS				1
#define _ENABLE_SHIPS							1
#define _ENABLE_CARS							1
#define _ENABLE_TANKS							1
#define _ENABLE_MOTORCYCLES				1
#define _ENABLE_PARACHUTES				1
#define _ENABLE_DATADISC					1
#define _ENABLE_VBS								0
#define _DISABLE_CRC_PROTECTION   1
#define _FORCE_DS_CONTEXT         0
#define _ENABLE_GAMESPY           0
#define _ENABLE_NEWHEAD           1
#define _ENABLE_SKINNEDINSTANCING 0
#define _ENABLE_BB_TREES          0
#define _ENABLE_CONVERSATION      1
#define _ENABLE_SPEECH_RECOGNITION  0
#define _ENABLE_EDITOR2           0
#define _ENABLE_EDITOR2_MP        0
#define _ENABLE_IDENTITIES        1
#define _ENABLE_INVENTORY         0
#define _ENABLE_MISSION_CONFIG    1
#define _ENABLE_INDEPENDENT_AGENTS  1
#define _ENABLE_DIRECT_MESSAGES   0
#define _ENABLE_FILE_FUNCTIONS    0
#define _ENABLE_HAND_IK           1
#define _ENABLE_WALK_ON_GEOMETRY  0
#define _ENABLE_BULDOZER          0
#define _ENABLE_COMPILED_SHADER_CACHE 1
#define _ENABLE_DISTRIBUTIONS     0
#define _USE_FCPHMANAGER          1
#define _ENABLE_DX10              0
#define _USE_BATTL_EYE_SERVER     1
#define _USE_BATTL_EYE_CLIENT     1
#define _ENABLE_TEXTURE_HEADERS   1 //if reading texture headers from texture headers file is enabled

//Heli sim

#define _HELISIM_ROTORLIB             1
#define _DUMMY_ROTORLIB               0
#define _HELISIM_ROTORLIB_SIM_THREAD  0
#define RTDYNAMICS_USE_STATIC_LIBRARY

#define _HELISIM_LARGE_OBJECTID       1   //VBS3 merge
#define _HELISIM_RENDER_TO_TEXTURE    1
#define _HELISIM_VIEW_DIST            1
#define _HELISIM_MAPS                 1
#define _HELISIM_PRECISE_INTERACTION  1
#define _HELISIM_ROPESIM              1
#define _HELISIM_IMPERIAL_SYSTEM      1
#define _HELISIM_FAKE_NIGHT_LIGHTS    1
#define _HELISIM_SOUNDS               1
#define _HELISIM_AMBIENT_TRAFFIC      1
#define _HELISIM_SHADOW_ZDISTANCE     1

//#define _HELISIM_OFFTREEREGION_64   1   // defined in fileServerAsync.hpp

#define _VBS3_MORETERRAINLODS         1   //VBS3 -> Helisim merge - !!! tersynth is only hotfixed  (data for VertexLodHeightsFromArray are too small)
#define _VBS3_LAND_SHADOW             1   //VBS3 -> Helisim merge
#define _VBS3_LARGE_LANDSEG           1   //VBS3 -> Helisim merge
#define _VBS3_NO_SEABED               1   //VBS3 -> Helisim merge - !!! tersynth is not cashed, yet to be done - shader update 
#define _VBS3_LARGE_WATERSEG          0   //VBS3 -> Helisim merge
#define _VBS3_LARGE_OBJECTS_WRP       0   //VBS3 -> Helisim merge, Storing of large static objects R-tree in wrp.
#define _VBS3_LARGE_OBJECTS           1   //VBS3 -> Helisim merge, Allows objects larger than 50m.

#if _BINARIZE
#define _BINARIZE_REDUCELODSHAPE    1
#define _BINARIZE_REDUCEOBJECT      0
#else
#define _BINARIZE_REDUCELODSHAPE    0
#define _BINARIZE_REDUCEOBJECT      0
#endif

/// plaform specific performace libraries disabled by default
#define _ENABLE_PERFLIB           0

#define _TIME_ACC_MIN							1.0
#define _TIME_ACC_MAX							4.0

#define _MACRO_CDP							  0

// default mod path 
#define _MOD_PATH_DEFAULT "CA;Expansion;tkoh_preview"
#define _SHOW_ONLY_CA_SERVERS     1

/// HDR improvements
#define _ARMA3_HDR_IMPROVEMENTS 1
/// changes in haze computation
#define _ARMA3_HAZE_COMPUTATION 1
/// simul weather
//#define _ARMA3_SIMUL_WEATHER 1
/// asynchronous cloud rendering (cloud geometry is created in vis thread)
/// disable this, if there are some problems with cloud rendering
/// LUKE - it is disabled now, because it is not working correctly
//#define _ARMA3_SIMUL_WEATHER_ASYNC_RENDER (_ARMA3_SIMUL_WEATHER && 0)
/// vertex buffer ex - store actual number of primitves to draw along with vertex buffer
/// needed for async rendering, but must be always enabled when _ARMA3_SIMUL_WEATHER is true 
//#define _ARMA3_VERTEX_BUFFER_EX _ARMA3_SIMUL_WEATHER

// new environment maps
#define _ARMA3_ENV_MAPS 1

// CD Key parameters
// new Product Key algorithm
#define NEW_KEYS 1

// number of bits for a distribution id
#define DISTR_BITS 6
// number of bits for a simple hash
#define SIMPLE_HASH_BITS 4
// number of bits for a player id
#define PLAYER_BITS (32 - DISTR_BITS - SIMPLE_HASH_BITS)

const bool CDKeyRegistryUser = false;
const char CDKeyRegistryPath[] = ConfigApp;

// ArmA 2 distributions
enum DistributionId
{
  DistTKOHUS,
  DistTKOHEnglish,
  DistTKOHGerman,
  DistTKOHPolish,
  DistTKOHRussian,
  DistTKOHCzech,
  DistTKOHEuro,
  DistTKOHInternational,
  DistTKOHJapanese,
  DistTKOHInternal,
  DistTKOHItalian,
  DistTKOHSpanish,
  NDistributions
};

// ArmA 2 distributions
enum ProductId
{
  ProductA2,
  ProductOA,
  ProductBAF,
  ProductPMC,
  ProductFree,
  ProductRFT,
  ProductTKOH,
  ProductOADECADE,
  NProducts
};

// maybe the product can have multiple productIDs, use macro factory
#if REINFORCEMENTS
#define PRODUCT_ID(XX) \
  XX(ProductRFT)
#else 
#define PRODUCT_ID(XX) \
  XX(ProductOA) \
  XX(ProductOADECADE)
#endif

#define CDKEY_BLACKLIST(XX) 

// CD/DVD protection
#ifndef _SECUROM
  #undef _SECUROM
#endif
#define _SECUROM 0
