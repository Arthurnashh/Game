// AI - general implementation parameters

///////////////////////////////////////////////////////////////////////////////
// Parameters

// small float
#define EPS 0.0001F

// generation of info
#if !_RELEASE
	#define LOG_STRAT						0		// write strategic maps
	#define LOG_POSITION_PROBL	0		// write searching nearest empty at strategic maps
	#define LOG_MAPS						0		// write operative maps (0..2)
	#define LOG_PROBL						0		// write operative maps when path was not found
	#define LOG_PATH						0		// write path as list
	#define LOG_THINK						0		// write calling of think
	#define LOG_COMM						0		// write communication
	#define LOG_GETINOUT				0		// write info about getting in / getting out
	#define SHOW_INFO						0
#else
	#define LOG_MAPS						0		// write operative maps (0..2)
	#define LOG_STRAT						0		// write strategic maps
	#define LOG_POSITION_PROBL	0		// write searching nearest empty at strategic maps
#endif

//#define MapCoord WORD
#define MapCoord int

#define DIST_MIN_OPER (2*LandGrid) // minimal planned length of oper. path
#define DIST_MAX_OPER (1.2f*LandGrid) // maximal used length of oper. path

extern int directions20[20][3];

#define FIRST_REPORT_TIME		5.0F
#define REPEAT_REPORT_TIME	180.0F

#define FIRST_AWAY_TIME		5.0F
#define REPEAT_AWAY_TIME	60.0F

#define DIST_UPDATE_CHECKPOINT	100.0F

