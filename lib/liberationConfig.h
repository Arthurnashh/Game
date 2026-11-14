// Configuration parameters for ArmA2Free
// - public SP & MP demo

#define _VERIFY_KEY								1
#define _VERIFY_CLIENT_KEY  			1  // server is checking the CD key of clients on GameSpy
#define _VERIFY_BLACKLIST					1
#define _ENABLE_EDITOR						1
#define _ENABLE_WIZARD						1
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
#define _ENABLE_DEDICATED_SERVER	1
#define _ENABLE_HOST              1
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
#define _ENABLE_DISTRIBUTIONS     1
#define _USE_FCPHMANAGER          1
#define _ENABLE_DX10              0
#define _USE_BATTL_EYE_SERVER     0
#define _USE_BATTL_EYE_CLIENT     0
#define _ENABLE_STEAM             0
#define _ENABLE_TEXTURE_HEADERS   0 //if reading texture headers from texture headers file is enabled

#define _TIME_ACC_MIN							1.0
#define _TIME_ACC_MAX							4.0

#define _MACRO_CDP							  0

// default mod path 
#define _MOD_PATH_DEFAULT "CA;Expansion"
#define _SHOW_ONLY_CA_SERVERS     1

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

// Testing of initial signatures test
#define _ACCEPT_ONLY_SIGNED_DATA  1
#define _LOAD_ONLY_SIGNED_DATA    1

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


// maybe the product can have multiple productIDs, use macro factory
#define PRODUCT_ID(XX) \
  XX(ProductLiberation)

// list of blacklisted cd keys in the form: XX(id1) XX(id2) ...
#define CDKEY_BLACKLIST(XX) \

// Accepted public keys
#define ACCEPTED_KEYS { \
  { \
  "liberation", \
    { \
    0x06, 0x02, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x52, 0x53, 0x41, 0x31, \
    0x00, 0x04, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x49, 0x1F, 0x7F, 0xD2, 0x51, 0xEB, 0xA9, 0xC7, \
    0x5D, 0x46, 0xF5, 0x0F, 0x80, 0x12, 0xAE, 0x8A, 0xFE, 0x58, 0xB9, 0xEB, 0x7D, 0x27, 0x31, 0x5F, \
    0xA7, 0x78, 0x96, 0xF6, 0x82, 0xC1, 0x8B, 0xA5, 0xE5, 0xB4, 0x7D, 0xEB, 0x9C, 0x0C, 0x70, 0x59, \
    0xE9, 0x9B, 0xAB, 0x4E, 0x6F, 0x55, 0xC7, 0xB7, 0xAD, 0xB3, 0x26, 0x62, 0x87, 0xF0, 0x23, 0xB1, \
    0x0C, 0x98, 0x48, 0xD6, 0x54, 0x12, 0x7B, 0x21, 0xC6, 0x38, 0xFC, 0x99, 0xF5, 0xB9, 0xAA, 0xD4, \
    0x56, 0x44, 0xED, 0xDD, 0xE2, 0x8B, 0x28, 0xD5, 0x8A, 0xF1, 0x24, 0x29, 0x8F, 0x73, 0x67, 0x27, \
    0x86, 0x6E, 0x24, 0xFC, 0x23, 0xE9, 0xD3, 0x18, 0x76, 0x14, 0xB5, 0x02, 0x4C, 0x16, 0xE5, 0xB5, \
    0x47, 0xE4, 0xCB, 0x73, 0x9D, 0x29, 0x6B, 0xEC, 0x76, 0x22, 0x85, 0xE3, 0xBF, 0x07, 0x57, 0xC3, \
    0x22, 0xA7, 0x48, 0x4F, 0xA2, 0xE9, 0xC5, 0xD6} \
  } \
}

// Additional checked files
#define ADDITIONAL_SIGNED_FILES {0}
