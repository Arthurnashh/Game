#undef _VERIFY_KEY
#undef _VERIFY_CLIENT_KEY
#undef _VERIFY_BLACKLIST
#undef _ENABLE_EDITOR
#undef _ENABLE_WIZARD
#undef _ENABLE_DEDICATED_SERVER
#undef _ENABLE_GAMESPY
#undef _ENABLE_VBS
#undef _ENABLE_SKINNEDINSTANCING
#undef _ENABLE_BB_TREES
#undef _ENABLE_SPEECH_RECOGNITION
#undef _ENABLE_EDITOR2
#undef _ENABLE_INVENTORY
#undef _ENABLE_DIRECT_MESSAGES
#undef _ENABLE_FILE_FUNCTIONS
#undef _DISABLE_CRC_PROTECTION
#undef _ENABLE_PERFLIB
#undef _ENABLE_BULDOZER
#undef _ENABLE_STEAM
#undef _ENABLE_TEXTURE_HEADERS 

#undef _ENABLE_DISTRIBUTIONS
#undef _USE_FCPHMANAGER
#undef _ENABLE_DX10
#undef _USE_BATTL_EYE_SERVER 
#undef _USE_BATTL_EYE_CLIENT

#undef _ENABLE_BRIEF_GRP

#undef _MOD_PATH_DEFAULT

#undef _SECUROM

#define _VERIFY_KEY								0
#define _VERIFY_CLIENT_KEY  			0  // server is checking the CD key of clients on GameSpy
#define _VERIFY_BLACKLIST					0
#define _ENABLE_EDITOR						0
#define _ENABLE_WIZARD						1
#define _ENABLE_DEDICATED_SERVER	1
#define _ENABLE_GSPCONTROLLED_SERVER	0
#define _ENABLE_HOST              1
#define _ENABLE_GAMESPY           0
#define _ENABLE_VBS               0
#define _ENABLE_SKINNEDINSTANCING 0
#define _ENABLE_BB_TREES          0
#define _ENABLE_SPEECH_RECOGNITION  0
#define _ENABLE_EDITOR2           0
#define _ENABLE_EDITOR2_MP        0
#define _ENABLE_INVENTORY         0
#define _ENABLE_DIRECT_MESSAGES   0
#define _ENABLE_FILE_FUNCTIONS    0
#define _DISABLE_CRC_PROTECTION   1
#define _ENABLE_BULDOZER          0
#define _ENABLE_DISTRIBUTIONS     0
#define _USE_FCPHMANAGER          0
#define _ENABLE_DX10              0
#define _USE_BATTL_EYE_SERVER     1
#define _USE_BATTL_EYE_CLIENT     1
#define _ENABLE_TEXTURE_HEADERS   1 //if reading texture headers from texture headers file is enabled

#define _ENABLE_BRIEF_GRP					0

#define _MOD_PATH_DEFAULT "Xbox" // was "X" for OFP:Elite (Xbox)

#if _RELEASE && !_PROFILE
  #define _ENABLE_PERFLIB						0
#else
  #define _ENABLE_PERFLIB						1
#endif
