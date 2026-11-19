// Configuration parameters for internal release / debug version
#define _ENABLE_PHYSICS           1
#define _VERIFY_KEY								1
#define _VERIFY_CLIENT_KEY  			1  // server is checking the CD key of clients on GameSpy
#define _VERIFY_BLACKLIST					0
#define _ENABLE_EDITOR						0
#define _ENABLE_WIZARD						0
#define _ENABLE_ALL_MISSIONS			0
#define _ENABLE_MP								1
#define _ENABLE_AI								1
#define _ENABLE_CAMPAIGN					1
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
#define _ENABLE_VBS								1
#define _ENABLE_GAMESPY           1
#define _ENABLE_NEWHEAD           1
#define _ENABLE_SKINNEDINSTANCING 0
#define _ENABLE_BB_TREES          0
#define _ENABLE_CONVERSATION      1
#define _ENABLE_SPEECH_RECOGNITION  0
#define _ENABLE_EDITOR2_MP        0
#define _ENABLE_EDITOR2           1
#define _ENABLE_IDENTITIES        1
#define _ENABLE_INVENTORY         0
#define _ENABLE_MISSION_CONFIG    1
#define _ENABLE_INDEPENDENT_AGENTS  1
#define _ENABLE_DIRECT_MESSAGES   0
#define _ENABLE_FILE_FUNCTIONS    0
#define _ENABLE_HAND_IK           1
#define _ENABLE_WALK_ON_GEOMETRY  0
#define _ENABLE_BULDOZER          1
#define _ENABLE_COMPILED_SHADER_CACHE 1
#define _ENABLE_DISTRIBUTIONS     0
#define _USE_FCPHMANAGER          1
#define _ENABLE_DX10              0
#define _USE_BATTL_EYE_SERVER     0
#define _USE_BATTL_EYE_CLIENT     0
#define _ENABLE_SEND_UDP_MESSAGE  1
#define _ENABLE_TEXTURE_HEADERS   1 //if reading texture headers from texture headers file is enabled

#define _DISABLE_GUI							1 // no GUI (mouse, output, keyboard)

//DayZ

/// plaform specific performace libraries disabled by default
#define _ENABLE_PERFLIB           0

#define _TIME_ACC_MIN							(1.0 / 128.0)
#define _TIME_ACC_MAX							4.0

// default mod path 
#define _MOD_PATH_DEFAULT "CA;Expansion;BAF"

// SecuROM API protection
#define _SECUROM 1

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
  ProductLiberation,
  NProducts
};


// CD Key parameters
#if 0
  // test Retail key - copied from ResistanceConfig.h
  const bool CDKeyRegistryUser = false;
  const unsigned char CDKeyPublicKey[] =
  {
    0x11, 0x00, 0x00, 0x00,
    0x9F, 0x01, 0x51, 0x75, 0xBB, 0x99, 0xA8, 0xD7, 0x1B, 0xA1, 0x26, 0x3D, 0x40, 0x83, 0xAF
  };
  const char CDKeyFixed[] = "ArmAssault";
  const int CDKeyIdLength = (sizeof(CDKeyPublicKey) / sizeof(*CDKeyPublicKey) - 4) - strlen(CDKeyFixed);
#elif 0
  // use developer key
  const bool CDKeyRegistryUser = true;
  const unsigned char CDKeyPublicKey[] =
  {
    0x11, 0x00, 0x00, 0x00,
    0x2F, 0xAD, 0xE2, 0xFA, 0x1A, 0xA2, 0x60, 0x95, 0xEB, 0x81, 0x76, 0xDB, 0xCC, 0xE3, 0x50
  };
  const char CDKeyFixed[] = "BIS Internal.";
  const int CDKeyIdLength = (sizeof(CDKeyPublicKey) / sizeof(*CDKeyPublicKey) - 4) - strlen(CDKeyFixed);
#elif 0 // CDKey test for Debugging purposes, to test CDKeys in release configuration
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

// ArmA 2 distributions
enum DistributionId
{
  DistArmA2OAUS,
  DistArmA2OAEnglish,
  DistArmA2OAGerman,
  DistArmA2OAPolish,
  DistArmA2OARussian,
  DistArmA2OACzech,
  DistArmA2OAEuro,
  DistArmA2OAHungarian,
  DistArmA2OABrasilian,
  DistArmA2OAInternal,
  NDistributions
};

// maybe the product can have multiple productIDs, use macro factory
// note: ProductBAF is used only for debugging purposes and to show how to configure it. In internal version it does not matter what is defined here...
#define PRODUCT_ID(XX) \
  XX(ProductBAF) \
  XX(ProductOA)

#else 
  // internal keys are needed mostly for two reasons
  // a) no need to have the CDKEY installed in registry (no setup with given product needed to be installed)
  // b) multiple instances of game can run simultaneously to test MP
  #define INTERNAL_KEYS          1

#endif
