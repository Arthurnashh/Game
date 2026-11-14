#ifdef _MSC_VER
#pragma once
#endif

#ifndef _GLOBAL_HPP
#define _GLOBAL_HPP

#include <Es/Framework/debugLog.hpp>
#include <Es/Strings/rString.hpp>

#include <El/Time/time.hpp>
#include <El/ParamFile/paramFileDecl.hpp>
#include <El/ParamArchive/serializeClass.hpp>
#include <El/Enum/dynEnum.hpp>
#include <El/Stringtable/stringtable.hpp>

#include "VideoOptionsConfig.hpp"

// global variables
// all simulated items have access to them and can use them freely

#define SHOW_TITLES_DEFAULT true
#define VOICE_THROUGH_SPEAKERS_DEFAULT false
#define PITCH_DEFAULT 1.0f
#define IGUI_DEFAULT_SCALE 0.85f
#define IGUI_DEFAULT_SCALE_XBOX 1.0

#if _VBS2 // additional VBS2 difficulty options
#define DIFFICULTY_TYPE_ENUM(YY,MessageName,XX) \
  YY(MessageName,Armor, IDS_DIFF_ARMOR,XX) \
  YY(MessageName,FriendlyTag, IDS_DIFF_FRIENDLY_TAG,XX) \
  YY(MessageName,EnemyTag, IDS_DIFF_ENEMY_TAG,XX) \
  YY(MessageName,HUD, IDS_DIFF_HUD,XX) \
  YY(MessageName,HUDPerm, IDS_DIFF_HUD_PERM,XX) \
  YY(MessageName,HUDWp, IDS_DIFF_HUD_WP,XX) \
  YY(MessageName,HUDWpPerm, IDS_DIFF_HUD_WP_PERM,XX) \
  YY(MessageName,AutoSpot, IDS_DIFF_AUTO_SPOT,XX) \
  YY(MessageName,Map, IDS_DIFF_MAP,XX) \
  YY(MessageName,WeaponCursor, IDS_DIFF_WEAPON_CURSOR,XX) \
  YY(MessageName,AutoGuideAT, IDS_DIFF_AUTO_GUIDE_AT,XX) \
  YY(MessageName,ClockIndicator, IDS_DIFF_CLOCK_INDICATOR,XX) \
  YY(MessageName,3rdPersonView, IDS_DIFF_3RD_PERSON_VIEW,XX) \
  YY(MessageName,Tracers, IDS_DIFF_TRACERS,XX) \
  YY(MessageName,UltraAI, IDS_ULTRA_AI,XX) \
  YY(MessageName,AutoAim, IDS_AUTO_AIM,XX) \
  YY(MessageName,UnlimitedSaves, IDS_DIFF_UNLIMITED_SAVES, XX) \
  YY(MessageName,DeathMessages, IDS_DIFF_DEATH_MESSAGES, XX) \
  YY(MessageName,NetStats, IDS_DIFF_NETWORK_STATS, XX) \
  YY(MessageName,VonID, IDS_DIFF_VON_ID, XX) \
  YY(MessageName,ExtendetInfoType, IDS_DIFF_LITE_UNITINFO, XX) \
  YY(MessageName,AllowSeagull, IDS_DIFF_ALLOW_SEAGULL, XX) \
  YY(MessageName,ShowArmAMap, IDS_DIFF_SHOW_ARMA_MAP, XX) \
  YY(MessageName,AIVoices,IDS_DIFF_AI_VOICES, XX) \
  YY(MessageName,AnalogThrottle,IDS_DIFF_ANALOG_THORTTLE, XX) \
  YY(MessageName,SoldierNames,IDS_DIFF_SOLDIER_NAME, XX) \
  YY(MessageName,UnlimitedFuel, IDS_DIFF_UNLIMITED_FUEL, XX) \
  YY(MessageName,SimplifiedHeliModel, IDS_DIFF_SIMPLE_HELI, XX) \
  YY(MessageName,FollowContour, IDS_DIFF_FOLLOW_CONTOUR, XX) \
  YY(MessageName,GrenadeTrajectory, IDS_DIFF_GREN_TRAJ, XX) \
  YY(MessageName,SuppressPlayer, IDS_DIFF_SUPPRESS_PLAYER, XX) \
  YY(MessageName,RealisticFatigue, IDS_DIFF_REAL_FATIGUE, XX) \
  YY(MessageName,Morale, IDS_DIFF_MORALE, XX) \
  YY(MessageName,StrongMorale, IDS_DIFF_STRONG_MORALE, XX) \
  YY(MessageName,LongTermMorale, IDS_DIFF_LONG_TERM_MORALE, XX) \
  YY(MessageName,SurrenderFriendly, IDS_DIFF_SURRENDER_FRIENDLY, XX) \
  YY(MessageName,SurrenderEnemy, IDS_DIFF_SURRENDER_ENEMY, XX)
#elif _HELISIM_ROTORLIB
#define DIFFICULTY_TYPE_ENUM(YY,MessageName,XX) \
  YY(MessageName,Armor, IDS_DIFF_ARMOR,XX) \
  YY(MessageName,FriendlyTag, IDS_DIFF_FRIENDLY_TAG,XX) \
  YY(MessageName,EnemyTag, IDS_DIFF_ENEMY_TAG,XX) \
  YY(MessageName,HUD, IDS_DIFF_HUD,XX) \
  YY(MessageName,HUDPerm, IDS_DIFF_HUD_PERM,XX) \
  YY(MessageName,HUDWp, IDS_DIFF_HUD_WP,XX) \
  YY(MessageName,HUDWpPerm, IDS_DIFF_HUD_WP_PERM,XX) \
  YY(MessageName,HUDGroupInfo, IDS_DIFF_HUD_GROUP_INFO, XX) \
  YY(MessageName,AutoSpot, IDS_DIFF_AUTO_SPOT,XX) \
  YY(MessageName,Map, IDS_DIFF_MAP,XX) \
  YY(MessageName,WeaponCursor, IDS_DIFF_WEAPON_CURSOR,XX) \
  YY(MessageName,AutoGuideAT, IDS_DIFF_AUTO_GUIDE_AT,XX) \
  YY(MessageName,ClockIndicator, IDS_DIFF_CLOCK_INDICATOR,XX) \
  YY(MessageName,3rdPersonView, IDS_DIFF_3RD_PERSON_VIEW,XX) \
  YY(MessageName,UltraAI, IDS_ULTRA_AI,XX) \
  YY(MessageName,AutoAim, IDS_AUTO_AIM,XX) \
  YY(MessageName,CameraShake, IDS_DIFF_CAMERA_SHAKE, XX) \
  YY(MessageName,UnlimitedSaves, IDS_DIFF_UNLIMITED_SAVES, XX) \
  YY(MessageName,DeathMessages, IDS_DIFF_DEATH_MESSAGES, XX) \
  YY(MessageName,NetStats, IDS_DIFF_NETWORK_STATS, XX) \
  YY(MessageName,VonID, IDS_DIFF_VON_ID, XX) \
  YY(MessageName,ExtendetInfoType, IDS_DIFF_LITE_UNITINFO, XX) \
  YY(MessageName,RoughLanding, IDS_DIFF_ROUGHLANDING, XX) \
  YY(MessageName,WindEnabled, IDS_DIFF_WINDENABLED, XX) \
  YY(MessageName,AutoTrimEnabled, IDS_DIFF_AUTOTRIMENABLED, XX) \
  YY(MessageName,StressDamageEnabled, IDS_DIFF_STRESSDAMAGEENABLED, XX)
#else
#define DIFFICULTY_TYPE_ENUM(YY,MessageName,XX) \
  YY(MessageName,Armor, IDS_DIFF_ARMOR,XX) \
  YY(MessageName,FriendlyTag, IDS_DIFF_FRIENDLY_TAG,XX) \
  YY(MessageName,EnemyTag, IDS_DIFF_ENEMY_TAG,XX) \
  YY(MessageName,HUD, IDS_DIFF_HUD,XX) \
  YY(MessageName,HUDPerm, IDS_DIFF_HUD_PERM,XX) \
  YY(MessageName,HUDWp, IDS_DIFF_HUD_WP,XX) \
  YY(MessageName,HUDWpPerm, IDS_DIFF_HUD_WP_PERM,XX) \
  YY(MessageName,HUDGroupInfo, IDS_DIFF_HUD_GROUP_INFO, XX) \
  YY(MessageName,AutoSpot, IDS_DIFF_AUTO_SPOT,XX) \
  YY(MessageName,Map, IDS_DIFF_MAP,XX) \
  YY(MessageName,WeaponCursor, IDS_DIFF_WEAPON_CURSOR,XX) \
  YY(MessageName,AutoGuideAT, IDS_DIFF_AUTO_GUIDE_AT,XX) \
  YY(MessageName,ClockIndicator, IDS_DIFF_CLOCK_INDICATOR,XX) \
  YY(MessageName,3rdPersonView, IDS_DIFF_3RD_PERSON_VIEW,XX) \
  YY(MessageName,UltraAI, IDS_ULTRA_AI,XX) \
  YY(MessageName,AutoAim, IDS_AUTO_AIM,XX) \
  YY(MessageName,CameraShake, IDS_DIFF_CAMERA_SHAKE, XX) \
  YY(MessageName,UnlimitedSaves, IDS_DIFF_UNLIMITED_SAVES, XX) \
  YY(MessageName,DeathMessages, IDS_DIFF_DEATH_MESSAGES, XX) \
  YY(MessageName,NetStats, IDS_DIFF_NETWORK_STATS, XX) \
  YY(MessageName,VonID, IDS_DIFF_VON_ID, XX) \
  YY(MessageName,ExtendetInfoType, IDS_DIFF_LITE_UNITINFO, XX)
#endif

#define DIFF_ENUM(MessageName,name,ids,XX) DT##name,

enum DifficultyType
{
  DIFFICULTY_TYPE_ENUM(DIFF_ENUM,ignored,ignored) \
  DTN
};


struct DifficultyDesc
{
	const char *name;
	int &desc;	// IDS of string - must be reference - IDS_ values are not longer constants but variables

	DifficultyDesc(const char *n, int &d)
		: desc(d)
	{
		name = n;
	}
};

#include "interpol.hpp"

class PartiallyLinearFunction
{
	AutoArray<float> _x;
	AutoArray<float> _y;

	public:
	PartiallyLinearFunction(){}
	~PartiallyLinearFunction(){}
  
  void Add(float x, float y)
  {
    _x.Add(x);
    _y.Add(y);
  }

  float Get(float x) const
  {
    return Lint(_x.Data(),_y.Data(),_x.Size(),x);
  }
  __forceinline float operator () (float x) const { return Get(x); }
};

#include "AI/abilities.hpp"

#ifndef DECL_ENUM_ABILITY_KIND
#define DECL_ENUM_ABILITY_KIND
DECL_ENUM(AbilityKind)
#endif

/// various settings different for each difficulty level
struct DifficultySettings
{
	RString displayName;
	RString description; // name of HTML file
	bool flags[DTN];
	bool flagsEnabled[DTN];
	bool showCadetHints;
	bool showCadetWP;
	float maxPilotHeight;
	float aiSkill[2]; // index - friendly(0)/enemy(1)
	float aiPrecision[2]; // index - friendly(0)/enemy(1)
	float myArmorCoef;
	float groupArmorCoef;
  float autoAimDistance; //!< max. distance from the target
  float autoAimSizeFactor; //!< how much size of the target is considered
  float autoAimAngle; //!< max. angular distance from the target
  /// strenght of peripheral vision, <0,1>
  float peripheralVisionAid;
  /// strengh of on-screen vision aid, <0,1>
  float visionAid;
  
	RString scoreImage;
	RString scoreChar;
  RString badScoreImage;
  RString badScoreChar;

  float recoilCoef;
  bool autoReload;
  float animSpeedCoef;

};
TypeIsMovableZeroed(DifficultySettings)

#ifndef _ENABLE_LIFE_CHEAT
  #if _ENABLE_CHEATS
    #define _ENABLE_LIFE_CHEAT 1
  #else
    #define _ENABLE_LIFE_CHEAT 0
  #endif
#endif

struct Config
{
  Config() : beLicenceAgreed(false) {}

	static const int heapSize = 9;
	static const int fileHeapSize = 64;
	/// max. distance for drawing landscape and objects
	float horizontZ,objectsZ,roadsZ;
#if _HELISIM_VIEW_DIST
  float vehiclesZ, treesZ, staticZ;
#endif
	float tacticalZ,radarZ; // visibility limits 
  static const float DefaultShadowZDistance;
  static const float MaxShadowZDistance;

private:
	float _shadowsZ; // max. distance for drawing shadows	
	float _invShadowsZ;
public:
	inline float GetMaxLinearDepth() const
	{
		return _shadowsZ;
	}

	inline float GetMaxInvLinearDepth() const
	{
		return _invShadowsZ;
	}

	inline float GetShadowZ() const
	{
		return _shadowsZ;
	}

	inline void SetShadowZ(float shadZ )
	{
		_shadowsZ = shadZ;
		_invShadowsZ = 1.0f/floatMax(1e-6, shadZ);
	}

	inline float GetInvShadowZ() const
	{
		return _invShadowsZ;
	}


	/// max. length of casted shadow
	float GetMaxShadowSize() const;

  void SetMaxShadowSize(float maxShadowsZ);
	
	/// max. distance of projected shadows
	float GetProjShadowsZ() const;
	
	float trackTimeToLive;
	float invTrackTimeToLive;
	float benchmark;

  #if _ENABLE_CHEATS
    /// toggle instancing on/off - useful for debugging
    bool enableInstancing;
  #else
    static const bool enableInstancing = true;
  #endif

  enum ThreadingMode
  {
    ThreadingNone,
    ThreadingEmulated,
    ThreadingReal
  };
  
  /// on PC we want _useCB to be optional because of configurations with 1 CPU only
  bool _useCB;
  /// was _useCB set explicitely or not?
  bool _CBExplicit;
  /// should D3D be acessed via dedicated background rendering thread?
  ThreadingMode _renderThread;
    
  /// should SW predication be used?
  bool _predication;
  
  #if _ENABLE_CHEATS
    bool _mtSerial;
    bool _mtSerialMat;
  #else
    static const bool _mtSerial = false;
    static const bool _mtSerialMat = false;
  #endif

  //@{ used for command line processing
  void Autodetect();
  void CPUCountChanged();
  void ForceCB();
  void DisableCB();
  //@}
  
	static const int maxLights = 32;
	static const int maxSounds = 16; //limit for 3D sounds (2D sounds has no limit, but the estimation is 16)
  static const int maxVoNSounds = 16; //limit for 2D and 3D VoN sounds together

	bool showTitles;
	bool singleVoice;

  //custom triplehead settings
  bool tripleHead;

  float IGUIScale;

  // user has agreed to BE licence
  bool beLicenceAgreed;

  float defaultAspect;
  enum IGUIScaleModes
  {
    ExtraSmallScale = 47,
    VerySmallScale = 55,
    SmallScale = 70,
    NormalScale = 85,
    LargeScale = 100
  };

#if _ENABLE_LIFE_CHEAT
	bool super;			//	player is indestructible
#endif

	//! show blood (level=1 moderate, level=2 full)
	int bloodLevel;

#if _HELISIM_IMPERIAL_SYSTEM
  // 0-use metric, 1 - use metric for ground, imperial for air, 2 - only imperial 
  int useImperialSystem;
#endif

#if _HELISIM_AMBIENT_TRAFFIC
  int ambientTraffic;
#endif

  // user-level head bob factor
  float headBob;
  // 0 old, 1 freelook, 2 freelook with actions
  int vehicleFreelook;
	
	int wantBpp,wantW,wantH; // preferred screen parameters
  
	// difficulties
	static DifficultyDesc diffDesc[DTN];

#if _VBS3
  float maxViewDistance;
  float maxObjDrawDistance;
#endif

#if _VBS3_CRATERS_LIMITCOUNT
  int maxPermanentCraters;
#endif

	int difficulty;
	int diffDefault;
	DynEnum diffNames;

  //full info type, or lite version
  bool useExtendetInfoType;

	AutoArray<DifficultySettings> diffSettings;
	
	const DifficultySettings *GetDifficultySettings() const;
	bool IsEnabled(DifficultyType type) const;
	float GetAISkill(int enemy) const;
	float GetAIPrecision(int enemy) const;
	bool ShowCadetHints() const;
	bool ShowCadetWP() const;
	RString GetScoreImage() const;
	RString GetScoreChar() const;
  RString GetBadScoreImage() const;
  RString GetBadScoreChar() const;
	float GetMyArmorCoef() const;
	float GetGroupArmorCoef() const;
  float GetAutoAimDistance() const;
  float GetAutoAimSizeFactor() const;
  float GetAutoAimAngle() const;
  float GetPeripheralVisionAid() const;
  float GetVisionAid() const;
  float GetRecoilCoef() const;
  bool GetAutoReload() const;
  float GetAnimSpeedCoef() const;
  float UseExtendetInfoType() const;
  bool RoughLandingEnabled() const;
  bool WindEnabled() const;
  bool AutoTrimEnabled() const;
  bool StressDamageEnabled() const;

  void SetUseExtendetInfoType(bool value);


  PartiallyLinearFunction _abilityCurve[NAbilityKind];

  float GetAbility(AbilityKind kind, float skill)  const
  {
    float ability = _abilityCurve[kind].Get(skill);
    return ( ability < FLT_MIN ? FLT_MIN : ability );
  }

#if !_BINARIZE
  void SetBELicence(bool agreed);
#endif

  void InitDifficulties();
	void LoadDifficulties(ParamEntryPar userCfg);
	void SaveDifficulties(ParamFile &userCfg);

  void LoadBESettings(ParamEntryPar userCfg);
  void SaveBESettings(ParamFile &userCfg);

  VideoOptionsConfig videoOptions;
  void LoadVideoOptionsConfig();
};

#ifndef DECL_ENUM_TARGET_SIDE
#define DECL_ENUM_TARGET_SIDE
DECL_ENUM(TargetSide)
#endif

#if _VBS3
struct UTMInfo{
  double easting;
  double northing;
  int zone;
  bool north;

  UTMInfo(double _easting, double _northing, int _zone, bool _north)
    :easting(_easting), northing(_northing), zone(_zone), north(_north){};
  void Reset()
  {
    easting = -1;
    northing = -1;
    zone = -1;
    north = false;
  }
  UTMInfo()
  {
   Reset();
  }
};
#endif

/** new values must be added to end, no values can be deleted - value serialized as integer */
#define AI_FEATURES_ENUM(type, prefix, XX) \
  XX(type, prefix, AwareFormationSoft) \
  XX(type, prefix, CombatFormationSoft) \

#ifndef DECL_ENUM_AI_FEATURES
#define DECL_ENUM_AI_FEATURES
DECL_ENUM(AIFeature)
#endif
DECLARE_ENUM(AIFeature, AIF, AI_FEATURES_ENUM)

struct GameHeader
{
	static const float MinDOFFocusDistance;
	static const float MaxDOFFocusDistance;

	// game info	
	char filename[80];
	char worldname[80];
	RString filenameReal;

	// player info
private:
#if _GAMES_FOR_WINDOWS
  // name of player profile is used instead
#else
  RString playerName;
#endif
public:
	RString playerFace;
	RString playerGlasses;
  /// class of speaker (see CfgVoiceTypes)
	RString playerSpeakerType;
	float playerPitch;
	RString voiceMask;
	TargetSide playerSide;
  bool playerNameChanged;
  /// voice should be directed to the speakers
  bool voiceThroughSpeakers;
  /// which new AI features are enabled
  int aiFeatures;
  
  GameHeader() {aiFeatures = ~0;}
  bool CheckAIFeature(AIFeature feature) const {Assert(aiFeatures!=~0);return (aiFeatures&(1<<feature))!=0;}
  void EnableAIFeature(AIFeature feature) {Assert(aiFeatures!=~0);aiFeatures |= (1<<feature);}
  void DisableAIFeature(AIFeature feature) {Assert(aiFeatures!=~0);aiFeatures &= ~(1<<feature);}
  
#if _GAMES_FOR_WINDOWS
  RString GetPlayerName() const;
#else
  RString GetPlayerName() const {return playerName;}
#endif

  void SetPlayerName(RString name);

#if _VBS2 // VBS2 nickname
  // in VBS2 player name is service ID number, so additional name is required
  RString playerHandle;
#endif
#if _VBS3 
  //UTM info gets stored here (based on the world file (see world.cpp)), can be changed through script
  //don't change it directly, call GWorld->SetUtmInfo instead!
  UTMInfo utmInfo;
#endif
};

const float OneDay=1.0f/365;
const float OneHour=OneDay/24;
const float OneMinute=OneHour/60;
const float OneSecond=OneMinute/60;

struct tm;

//! This class encapsulates block of memory and introduces methods for simple transfer from/to them
class SimpleStream : public AutoArray<char>
{
protected:
  //! read / write position
  int _pos;

public:
  //! Constructor
  SimpleStream() {_pos = 0;}

  //! Return read / write position
  int GetPos() const {return _pos;}

  //! Write block of memory
  /*!
  \param buffer data to write
  \param size size of data
  */
  void Write(const void *buffer, int size)
  {
    int minSize = _pos + size;
    if (Size() < minSize) Resize(minSize);
    memcpy(Data() + _pos, buffer, size);
    _pos += size;
  }
  //! Read block of memory
  /*!
  \param buffer buffer for read data
  \param size size of data to read
  */
  bool Read(void *buffer, int size)
  {
    int minSize = _pos + size;
    if (Size() < minSize) return false;
    memcpy(buffer, Data() + _pos, size);
    _pos += size;
    return true;
  }
  //! Write string
  /*!
  \param str string to write
  */
  void WriteString(RString str)
  {
    int size = str.GetLength() + 1;
    int minSize = _pos + size;
    if (Size() < minSize) Resize(minSize);
    memcpy(Data() + _pos, str, size);
    _pos += size;
  }
  //! return read string
  RString ReadString()
  {
    int size = strlen(Data() + _pos) + 1;
    if (Size() < _pos + size) return "";
    RString result = Data() + _pos;
    _pos += size;
    return result;
  }

  LocalizedString ReadLocalizedString()
  {
    char type;
    if (Read(&type,sizeof(type)))
    {
      RString id = ReadString();
      return LocalizedString(LocalizedString::Type(type),id);
    }
    return LocalizedString();
  }
  void WriteLocalizedString(const LocalizedString &str)
  {
    char type = str._type;
    Write(&type,sizeof(type));
    WriteString(str._id);
  }
};

/// real time in the world
class Clock : public SerializeClass
{
protected:
  // time represented as multiplier of 1/50 second in the year
	static const int timeInYear32Scale = 365 * 24 * 60 * 60 * 50;
	static const int timeInYear32OneDay = 24 * 60 * 60 * 50;
	/// use timeInYear32Scale representation
	int _timeInYear32;
	/// derived from _timeInYear32 - 1.0 is one year
	float _timeInYear;
	/// actual time relative to _timeInYear32 - _timeInYear32 precision is too small
	float _changeTime;
	/// relative time in day (1.0 is one day)
	float _timeOfDay;	
	int _year;

	float _timeMultiplier;

public:
	Clock(): _timeMultiplier(1.0f)
	{
		SetTime(10 * OneHour, 120 * OneDay, 1985);
	}

	float GetTimeInYear() const {return _timeInYear;}
	float GetTimeOfDay() const {return _timeOfDay;}
	int GetYear() const {return _year;}

  void SetTime(float time, float date, int year);

	bool AdvanceTime( float deltaT );

  void GetDate(tm &tmDate);
  static void FormatDate(const char *format, char *buffer, const tm &tmDate);

  void FormatDate(const char *format, char *buffer);
	static float ScanDateTime(const char *date, const char *time, int &year);
  static float ScanDateTime(int year, int month, int day, int hour, int min);

	#ifndef ACCESS_ONLY
	LSError Serialize(ParamArchive &ar);
	#else
	LSError Serialize(ParamArchive &ar) {return LSOK;}
	#endif
  void Read(SimpleStream &in);
  void Write(SimpleStream &out);

	void SetTimeMultiplier(float timeMul)
	{
		_timeMultiplier = floatMinMax(timeMul, 0.1f, 64.0f);
	}

	float GetTimeMultiplier() const
	{
		return _timeMultiplier;
	}
};

//#include "randomGen.hpp"
//#include <El/FileServer/fileServer.hpp>

DWORD GlobalTickCount();

#if _ENABLE_REPORT
struct TimeScope
{
  DWORD start;
  const char *name;
  DWORD minToLog;
  DWORD toBreak;
  
  TimeScope(const char *name, DWORD minToLog=0, DWORD toBreak=UINT_MAX)
  :minToLog(minToLog),toBreak(toBreak),name(name),start(::GlobalTickCount()){}
  ~TimeScope()
  {
    DWORD time = GlobalTickCount()-start;
    if (time>=minToLog)
    {
    LogF("%s took %d ms",name,time);
      if (time>toBreak)
      {
        BreakIntoDebugger("time limit hit");
  }
    }
  }
};
#else
struct TimeScope
{
  TimeScope(const char *name, DWORD minToLog=0, DWORD toBreak=UINT_MAX){}
  ~TimeScope(){}
};
#endif

//bool IsOutOfMemory();

/// default drive side on the road
DEFINE_ENUM_BEG(DriveOnRoadSide)
  RSCenter,
  RSRight,
  RSLeft,
DEFINE_ENUM_END(DriveOnRoadSide)

struct Globals
{
  /// time in the mission
	Time time;
	/// UI time - never accelerated, not restarted with a mission
	UITime uiTime;
	/// number of the current frame, not restarted with a mission
	int frameID;
	
	/// game time as time and date
	Clock clock;

	/// clock for weather system
	Clock clockWeather;

	Time GetTime() const {return time;}

	Config config;
	GameHeader header;
	
	void InitPhase1();
	void InitPhase2();

	void Clear();

	// pause menu commands
  bool exit;

  //! Demo version (Xbox - E3 demo)
  bool demo;

  // preread value of isDayZ = (Pars>>"CfgPatches").FindEntry("dayz");
  // it is used from different threads, ParamFile usage from diff thread has caused crash (news:k1nf0e$hrn$1@new-server.localdomain) 
  enum IsDayZ {NotDayZ,IsDayZ,IsDayZOther} isDayZ;

#if _VBS2 
  bool vbsAdminMode; // vbs admin mode
  bool vbsAdminUser; // VBS2 was started in admin mode?
  bool vbsEnableCNR; //Is CNR SIM enabled
  bool vbsEnableLVC; //commandLine requests LVC
  DriveOnRoadSide ActiveRoadSide; // default drive side on the road

  Globals() 
  {
# if _VBS2_LITE
    vbsAdminMode = vbsAdminUser = true;
# else
    vbsAdminMode = vbsAdminUser = false;
# endif
    vbsEnableCNR = false;
    vbsEnableLVC = false;
    ActiveRoadSide = RSRight;
  }
#endif
  
#if _VBS2 // autoassign
  RString autoAssign;
  RString autoAssignGrp;
  TargetSide autoAssignSide;
#endif

#if _VBS2 // player color
  RString playerColor;
#endif

#if _VBS2 // version number is available for any display
  RString buildVersion;
  RString buildDate;
#endif

#if _VBS3
  RString expiryDate; //written by trial.hpp
#endif
};

#include <Es/Framework/appFrame.hpp>

void GFileServerStart();
void GFileServerStop();
void GFileServerFlush();
void GFileServerClear();
void GFileServerUpdate();
void GFileServerSubmitRequestsMinimizeSeek();
void GFileServerWaitForOneRequestDone(int timeout);

RString GetPublicKey();
RString GetProductIdString();

template <class BaseAllocator=MemAllocD>
class SecuredAllocator: public BaseAllocator
{
  public:
  void Free( void *mem, size_t size )
  {
    memset(mem,0,size);
    BaseAllocator::Free(mem,size);
  }
};

AutoArray<unsigned char, SecuredAllocator<> > GetCDKeyBinary();

/// check product - does not work with internal keys
int GetProductId();
/// restrict features by a product - works for internal version as
bool CheckProductId(int productId);
RString CheckLanguage(RString language);
#ifdef _WIN32
HRESULT FindConfigAppRegistry(HKEY &hKey);
#endif

//! get most critical error level since last ResetErrors
ErrorMessageLevel GetMaxError();

//! reset error state - used in conjunction with GetMaxError()
void ResetErrors();

//! get most critical error message since last ResetErrors
RString GetMaxErrorMessage();

extern Globals Glob;

enum TimeStampFormat { TSFNone, TSFShort, TSFFull };
extern TimeStampFormat GTimeStampFormat;

#if _ENABLE_DEDICATED_SERVER
  bool IsDedicatedServer();
#else
  static inline bool IsDedicatedServer() {return false;}
#endif
  bool IsDedicatedClient();

#endif

