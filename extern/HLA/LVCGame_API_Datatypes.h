#ifndef __Callbacks___
#define __Callbacks___

// ===================================
// Unit Side 
// ===================================
#define UNIT_SIDE_TYPE(XX)\
	XX(OpFor) \
	XX(BlueFor) \
	XX(Resistance) \
	XX(NonCombatant)
#define US_DEFINE_ENUM(name) US_##name,
enum UnitSide{
	UNIT_SIDE_TYPE(US_DEFINE_ENUM)
	US_NONE	
};
#define US_ENUM_Name(name) #name,
static const char *UnitSideNames[] = {
	UNIT_SIDE_TYPE(US_ENUM_Name)
};

enum SimStateData
{
	//! no SIM
	SNone,
	//! VBS is creating (message formats are not registered yet)
	SCreating,
	//! VBS is created, messages can be sent
	SCreate,
	//! unit is logged into sim (his identity is created)
	SLogin,
	//! server is editing mission
	SEdit,
	//! unit voted mission
	SMissionVoted,
	//! unit are assigned to sides
	SPrepareSide,
	//! unit are assigned to roles
	SPrepareRole,
	//! unit is already assigned
	SPrepareOK,
	//! mission debriefing
	SDebriefing,
	//! mission debriefing is already read
	SDebriefingOK,
	//! transfering mission file
	STransferMission,
	//! loading island
	SLoadIsland,
	//! briefing screen
	SBriefing,
	//! Sim is running
	SSimulation
};

enum EPosInfo
{
    Pos_None = 0,
    Pos_Driver,
    Pos_Turret,
    Pos_Cargo
};

enum StanceType
{
    Stance_None = 0,
    Stance_StandingStill,
    Stance_StandingWalk,
    Stance_StandingRun,
    Stance_Kneeling,
    Stance_Prone,
    Stance_Crawling,
    Stance_Swimming,
    Stance_Parachuting,
    Stance_Jumping,
    Stance_Sitting,
    Stance_Squatting,
    Stance_Crouching,
    Stance_Wading,
    Stance_Surrendering,
    Stance_Detained
};

enum WeaponPostureType
{
    WeaponPosture_None = 0,
    WeaponPosture_Stowed,
    WeaponPosture_Deployed,
    WeaponPosture_Raised
};

enum ComplianceType
{
    Compliance_None = 0,
    Compliance_Detained,
    Compliance_Surrender,
    Compliance_UsingFists,
    Compliance_VerbalAbuse1,
    Compliance_VerbalAbuse2,
    Compliance_VerbalAbuse3,
    Compliance_PassiveResistance1,
    Compliance_PassiveResistance2,
    Compliance_PassiveResistance3,
    Compliance_NonLethalWeapon1,
    Compliance_NonLethalWeapon2,
    Compliance_NonLethalWeapon3,
    Compliance_NonLethalWeapon4,
    Compliance_NonLethalWeapon5,
    Compliance_NonLethalWeapon6
};

enum HatchStateType
{
    HatchState_None = 0,
    HatchState_Closed,
    HatchState_Popped,
    HatchState_PoppedPersonVisible,
    HatchState_Open,
    HatchState_OpenPersonVisible
};

enum CamouflageType
{
    CamouflageType_None = 0,
    CamouflageType_Desert,
    CamouflageType_Winter,
    CamouflageType_Woodland,
    CamouflageType_Generic
};

enum TrailingEffectsType
{
    TrailingEffects_None = 0,
    TrailingEffects_Small,
    TrailingEffects_Medium,
    TrailingEffects_Large
};

// Appearance Bit Masks
static const unsigned int LVC_FIREPOWER_DISABLED    = 0x0001;
static const unsigned int LVC_MOBILITY_DISABLED     = 0x0002;
static const unsigned int LVC_DESTROYED             = 0x0004;
static const unsigned int LVC_SMOKE_PLUME           = 0x0008;
static const unsigned int LVC_ENGINE_SMOKE          = 0x0010;
static const unsigned int LVC_FLAMES_PRESENT        = 0x0020;
static const unsigned int LVC_POWER_PLANT_ON        = 0x0040;
static const unsigned int LVC_CONCEALED             = 0x0080;
static const unsigned int LVC_TENT_DEPLOYED         = 0x0100;
static const unsigned int LVC_RAMP_DEPLOYED         = 0x0200;
static const unsigned int LVC_LAUNCHER_FLASH        = 0x0400;
static const unsigned int LVC_LAUNCHER_RAISED       = 0x0800;
static const unsigned int LVC_AFTERBURNER_ON        = 0x1000;

// Light Bit Masks
static const unsigned int LVC_LIGHT_HEAD            = 0x0001;
static const unsigned int LVC_LIGHT_TAIL            = 0x0002;
static const unsigned int LVC_LIGHT_BRAKE           = 0x0004;
static const unsigned int LVC_LIGHT_BLACKOUT        = 0x0008;
static const unsigned int LVC_LIGHT_BLACKOUT_BRAKE  = 0x0010;
static const unsigned int LVC_LIGHT_RUNNING         = 0x0020;
static const unsigned int LVC_LIGHT_SPOT            = 0x0040;
static const unsigned int LVC_LIGHT_FLASHLIGHT      = 0x0080;
static const unsigned int LVC_LIGHT_INTERIOR        = 0x0100;
static const unsigned int LVC_LIGHT_LANDING         = 0x0200;
static const unsigned int LVC_LIGHT_NAV             = 0x0400;
static const unsigned int LVC_LIGHT_ACL             = 0x0800;

struct EntityId
{
    int ApplicationNumber;
    int EntityNumber;
};

static const EntityId NullEntityId = {0,0};

struct Generic3DType
{
    float x;
    float y;
    float z;
};

typedef Generic3DType Coordinate3DType;
typedef Generic3DType Vector3DType;
typedef Generic3DType BoundingBoxType;

struct EntityCreateData
{
    EntityId Id;
    Coordinate3DType Pos;
    char EntityType[255];        // Entity type
    EntityId VehicleId;          // Id of vehicle, that this entity is in
    EPosInfo EPos;               // Entity position in vehicle
    BoundingBoxType Dimensions;  // Physical dimensions of the entity
    UnitSide Side;               // Entity side
    char Name[255];              // Entity name
    char Profile[255];           // Profile name
    int CrewIndex;               // Index inside the vehicle
};

struct CenterCreateData
{
  EntityId Id;
  UnitSide Side;               
};

struct GroupCreateData
{
  EntityId Id;
  EntityId CenterId;
  int Number;
  EntityId LeaderId;          //Unit Id
};

/*
struct GroupUpdataData
{
  EntityId Id;
  EntityId SubgroupId;
  EntityId LeaderId;
};
*/

struct UnitCreateData
{
  EntityId Id;
  EntityId PersonId;           // Person representing the unit
  EntityId GroupId;            // Person representing the unit
  int Number;
};

struct TurretUpdate
{
  EntityId Id;                // Vehicle ID
  int   TurretNum;            // Turrent Number    
  float Azimuth;              // Azimuth of the turret  
  float AzimuthWanted;        
  float Elevation;            // Elevation of the turret
  float ElevationWanted;      
};

struct EntityUpdateData
{
    EntityId Id;
    Coordinate3DType Pos;
    Vector3DType Dir;
    Vector3DType Up;
    Vector3DType Velocity;              // velocity m/s
    Vector3DType Acceleration;          // linear acceleration m/s^2
    float Damage;                       // % of entity that is damaged
    unsigned int Appearance;            // appearance bit fields
    unsigned int Lights;                // light state bit fields
    bool HasTurret;                     // Entity is articulated
    float Azimuth;	    	            // Relative Azimuth for turret radians (VBS Y rot)
    float Elevation;    	            // Relative Elevation for turret radians (VBS X rot)
    float AzimuthWanted;	    	    // Wanted Relative rotation from Azimuth
    float ElevationWanted;    	        // Wanted Relative Elevation from Elevation
    EntityId VehicleId;                 // Id of vehicle, that this entity is in
    EPosInfo EPos;                      // Entity position in vehicle
    StanceType Stance;                  // Stance for lifeforms as per enums
    WeaponPostureType PrWeaponPosture;  // Posture for primary weapon as per enums
    int CrewIndex;                      // Index inside the vehicle
    int SelectedWeapon;                 // Currently selected weapon
    char URN[12];                       // Unit Reference number
};

typedef EntityId EntityDeleteData;
typedef EntityId DesignatorDeleteData;
typedef EntityId GroupDeleteData;

struct DesignatorCreateData
{
    EntityId Id;                        // Designator ID
    EntityId DesignatingId;             // ID of the entity controlling the desig
    EntityId DesignatedId;              // ID of the designated entity (or 0,0)
    int DesignatorNameCode;             // 
    int DesignatorFunctionCode;         //
    float DesignatorPower;              // power in watts
    float DesignatorWavelength;         // wavelength in microns
    Coordinate3DType RelativePosition;  // desig pos relative to entity centre
    Coordinate3DType WorldPosition;     // desig pos in the world
    Vector3DType LinearAcceleration;    // linear accel in m/s^2
};

typedef DesignatorCreateData DesignatorUpdateData;

struct FireWeaponData
{
    EntityId FiringId;          // Must be known as this is direct fire!
    EntityId TargetId;          // if known -> otherwise 0,0
    EntityId MunitionId;        // if known -> otherwise 0,0
    int WeaponId;               // if known -> otherwise 0
    Coordinate3DType Pos;       // Initial position
    Vector3DType Velocity;      // Initial Velocity
    char MunitionType[255];     // MunitionType
    float TimeToLive;           // Time to live (in seconds)
};

struct DetonationData
{
    EntityId FiringId;	        // if known -> otherwise 0,0
    EntityId TargetId;          // if known -> otherwise 0,0
    EntityId MunitionId;        // if known -> otherwise 0,0
    Vector3DType Velocity;      // Impact velocity
    Coordinate3DType Pos;	    // Impact position
    Coordinate3DType RelPos;    // Impact position in entity, relative to its center
    char MunitionType[255];     // Ammunition type 
};

struct CNRLogEvent
{
	long time;					//Event start time
	int id;						//Event index number 
	long freq;					//Radio frequency the event was recorded at
	long duration;				//length of event
	char* source;				//sourc id the event was recorded at
};



struct DataScript
{
	EntityId OriginatingId;		//DIS Entity of the incoming setDataPDU
	EntityId ReceivingId;		//DIS Entity of the destination setDataPDU	
	int RequestId;				//Id of the Datum x in the setDataPDU
	char DatumValue[512];			//Datum script

};

#endif //__Callbacks___