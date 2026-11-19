// Configuration parameters for ArmA demo
// - public SP & MP demo
#define _VERIFY_KEY								0
#define _VERIFY_CLIENT_KEY  			0  // server is checking the CD key of clients on GameSpy
#define _VERIFY_BLACKLIST					0
#define _ENABLE_EDITOR						1
#define _ENABLE_WIZARD						1
#define _ENABLE_ALL_MISSIONS			0
#define _ENABLE_MP								1
#define _ENABLE_AI								1
#define _ENABLE_CAMPAIGN					0
#define _ENABLE_SINGLE_MISSION		1
#define _ENABLE_UNSIGNED_MISSIONS	0
#define _ENABLE_BRIEF_GRP					1
#define _ENABLE_PARAMS						1
#define _ENABLE_CHEATS						0
#define _ENABLE_ADDONS						1
#define _ENABLE_DEDICATED_SERVER	1
#define _ENABLE_GSPCONTROLLED_SERVER	0
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
#define _ENABLE_GAMESPY           1
#define _ENABLE_NEWHEAD           1
#define _ENABLE_SKINNEDINSTANCING 0
#define _ENABLE_BB_TREES          0
#define _ENABLE_CONVERSATION      1
#define _ENABLE_SPEECH_RECOGNITION  0
#define _ENABLE_EDITOR2           1
#define _ENABLE_EDITOR2_MP        0
#define _ENABLE_IDENTITIES        1
#define _ENABLE_INVENTORY         0
#define _ENABLE_MISSION_CONFIG    1
#define _ENABLE_INDEPENDENT_AGENTS  1
#define _ENABLE_OLD_AI            1
#define _ENABLE_DIRECT_MESSAGES   0
#define _ENABLE_FILE_FUNCTIONS    0
#define _ENABLE_HAND_IK           1
#define _ENABLE_BULDOZER          0
#define _ENABLE_COMPILED_SHADER_CACHE 1
#define _ENABLE_DISTRIBUTIONS     0
#define _USE_FCPHMANAGER          1
#define _ENABLE_DX10              0
#define _USE_BATTL_EYE_SERVER     1
#define _USE_BATTL_EYE_CLIENT     1
#define _ENABLE_TEXTURE_HEADERS   0 //if reading texture headers from texture headers file is enabled

//DayZ

//Heli sim

#define _HELISIM_ROTORLIB             1
#define _DUMMY_ROTORLIB               0
#define _HELISIM_ROTORLIB_SIM_THREAD  0
#define RTDYNAMICS_USE_STATIC_LIBRARY

#define _HELISIM_LARGE_OBJECTID       1   //VBS3 merge
#define _HELISIM_RENDER_TO_TEXTURE    1
#define _HELISIM_VIEW_DIST            1
#define _HELISIM_MAPS                 1
#define _HELISIM_PRECISE_INTERACTION  0
#define _HELISIM_IMPERIAL_SYSTEM      1
#define _HELISIM_FAKE_NIGHT_LIGHTS    1
#define _HELISIM_SOUNDS               1
#define _HELISIM_AMBIENT_TRAFFIC      0
#define _HELISIM_TERRAIN_RIVERS       1
#define _HELISIM_SHADOW_ZDISTANCE     1

//#define _HELISIM_OFFTREEREGION_64   1   // defined in fileServerAsync.hpp

#define _VBS3_MORETERRAINLODS         1   //VBS3 -> Helisim merge - !!! tersynth is only hotfixed  (data for VertexLodHeightsFromArray are too small)
#define _VBS3_LAND_SHADOW             1   //VBS3 -> Helisim merge
#define _VBS3_LARGE_LANDSEG           0   //VBS3 -> Helisim merge
#define _VBS3_NO_SEABED               1   //VBS3 -> Helisim merge - !!! tersynth is not cashed, yet to be done - shader update 
#define _VBS3_LARGE_WATERSEG          1   //VBS3 -> Helisim merge
#define _VBS3_LARGE_OBJECTS_WRP       0   //VBS3 -> Helisim merge, Storing of large static objects R-tree in wrp.
#define _VBS3_LARGE_OBJECTS           0   //VBS3 -> Helisim merge, Allows objects larger than 50m.

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

#define _ENABLE_AUTOTEST          0

// default mod path 
#define _MOD_PATH_DEFAULT "HSim"

/// new terrain decals
#define _ARMA3_TERRAIN_DECALS 1
/// HDR improvements
#define _ARMA3_HDR_IMPROVEMENTS 1
/// changes in haze computation
#define _ARMA3_HAZE_COMPUTATION 1
/// simul weather
#define _ARMA3_SIMUL_WEATHER 0
/// asynchronous cloud rendering (cloud geometry is created in vis thread)
/// disable this, if there are some problems with cloud rendering
/// LUKE - it is disabled now, because it is not working correctly
#define _ARMA3_SIMUL_WEATHER_ASYNC_RENDER (_ARMA3_SIMUL_WEATHER && 0)
/// vertex buffer ex - store actual number of primitves to draw along with vertex buffer
/// needed for async rendering, but must be always enabled when _ARMA3_SIMUL_WEATHER is true 
#define _ARMA3_VERTEX_BUFFER_EX _ARMA3_SIMUL_WEATHER
// new lighting
#define _ARMA3_NEW_LIGHTING_CFG 1
// new environment maps
#define _ARMA3_ENV_MAPS 1
// -------------------------------------------------------------
// Testing of initial signatures test
#define _ACCEPT_ONLY_SIGNED_DATA  1
// Accepted public keys
#define ACCEPTED_KEYS { \
  { \
  "TKOH_Demo", \
    { \
    0x06, 0x02, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x52, 0x53, 0x41, 0x31, \
    0x00, 0x04, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0xBF, 0x39, 0x18, 0x20, 0x90, 0x75, 0x82, 0xC4, \
    0x70, 0x66, 0xF1, 0x0C, 0x30, 0x1E, 0x35, 0xAE, 0x63, 0xEE, 0x7C, 0x91, 0x30, 0xA8, 0xAA, 0xAE, \
    0x4A, 0x45, 0x47, 0xB4, 0xC7, 0x54, 0x0E, 0x01, 0x28, 0x4D, 0x83, 0x09, 0x60, 0x20, 0xEC, 0x4F, \
    0x03, 0xBC, 0x2C, 0x3E, 0x26, 0x71, 0x64, 0x1A, 0x68, 0x98, 0x85, 0x3A, 0x3F, 0x66, 0x71, 0xC7, \
    0x88, 0x2A, 0xA4, 0x8E, 0x8F, 0x8C, 0x99, 0x03, 0x7D, 0x61, 0xAE, 0x39, 0x75, 0x12, 0x63, 0xC8, \
    0xDF, 0x9E, 0xBE, 0x94, 0x32, 0x91, 0xF8, 0xA8, 0x2D, 0x8D, 0x3E, 0x73, 0xD0, 0x5E, 0xB0, 0x23, \
    0xF4, 0x1F, 0xE6, 0xE8, 0x65, 0xEC, 0xC8, 0x53, 0x33, 0xC7, 0xBF, 0x1B, 0x26, 0x1D, 0x07, 0xEB, \
    0xD9, 0xF7, 0xCC, 0x46, 0xDA, 0xC1, 0x54, 0xE3, 0x33, 0xF6, 0xBC, 0xA0, 0x67, 0x3F, 0xE7, 0x17, \
    0x8A, 0xCA, 0x42, 0x05, 0xC0, 0x3D, 0x24, 0xAE} \
  } \
}

// Additional checked files
#define ADDITIONAL_SIGNED_FILES {0}