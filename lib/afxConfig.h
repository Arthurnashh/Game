// Configuration parameters for internal release / debug version
#define _VERIFY_KEY								1
#define _VERIFY_CLIENT_KEY  			1  // server is checking the CD key of clients on GameSpy
#define _VERIFY_BLACKLIST					0
#define _ENABLE_EDITOR						1
#define _ENABLE_WIZARD						1
#define _ENABLE_ALL_MISSIONS			1
#define _ENABLE_MP								1
#define _ENABLE_AI								1
#define _ENABLE_CAMPAIGN					1
#define _ENABLE_SINGLE_MISSION		1
#define _ENABLE_UNSIGNED_MISSIONS	1
#define _ENABLE_BRIEF_GRP					1
#define _ENABLE_PARAMS						1
#define _ENABLE_CHEATS						1
#define _ENABLE_ADDONS						1
#define _ENABLE_DEDICATED_SERVER	1
#define _ENABLE_GSPCONTROLLED_SERVER	0
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
#define _ENABLE_VBS								1
#define _ENABLE_NEWHEAD           0
#define _ENABLE_SKINNEDINSTANCING 0
#define _ENABLE_BB_TREES          0
#define _ENABLE_CONVERSATION      0
#define _ENABLE_SPEECH_RECOGNITION  0
#define _ENABLE_EDITOR2           0
#define _ENABLE_EDITOR2_MP        0
#define _ENABLE_IDENTITIES        0
#define _ENABLE_INVENTORY         0
#define _ENABLE_MISSION_CONFIG    0
#define _ENABLE_INDEPENDENT_AGENTS  0
#define _ENABLE_OLD_AI            0
#define _ENABLE_DIRECT_MESSAGES   0
#define _ENABLE_FILE_FUNCTIONS    0
#define _ENABLE_HAND_IK           0
#define _ENABLE_BULDOZER          0
#define _ENABLE_COMPILED_SHADER_CACHE 1
#define _ENABLE_DISTRIBUTIONS     0
#define _USE_FCPHMANAGER          1
#define _ENABLE_DX10              0
#define _USE_BATTL_EYE_SERVER     1
#define _USE_BATTL_EYE_CLIENT     1
#define _ENABLE_STEAM_OWNERSHIP   1

#define _TIME_ACC_MIN							(1.0 / 128.0)
#define _TIME_ACC_MAX							4.0

// Registry key for saving player's name, IP address of server etc.
//#define ConfigApp "Software\\Codemasters\\Operation Flashpoint"

// CD Key parameters
const bool CDKeyRegistryUser = false;
const unsigned char CDKeyPublicKey[] =
{
  0x13, 0x00, 0x00, 0x00,
  0xF7, 0xB0, 0x64, 0xD7, 0xDB, 0x87, 0x52, 0x27, 0x57, 0x91, 0xB1, 0x9E, 0x0E, 0x81, 0x88
};
const char CDKeyFixed[] = "Flashpoint";
const int CDKeyIdLength = (sizeof(CDKeyPublicKey) / sizeof(*CDKeyPublicKey) - 4) - strlen(CDKeyFixed);
