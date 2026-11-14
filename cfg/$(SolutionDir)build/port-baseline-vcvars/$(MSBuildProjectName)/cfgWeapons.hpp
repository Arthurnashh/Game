// ammunition

// define ammuniton types

#define WeaponNoSlot		0	// dummy weapons
#define WeaponSlotPrimary	1	// primary weapons
#define WeaponSlotSecondary	4	// secondary weapons
#define WeaponSlotHandGun	2	// HandGun
#define WeaponSlotHandGunItem	16 // HandGun magazines
#define WeaponSlotItem		256	// items
#define WeaponSlotBinocular	4096	// binocular
#define WeaponHardMounted	65536
#define WeaponSlotInventory 131072 // inventory items

#define TracerEColor 0.2,0.8,0.1
#define TracerWColor 0.8,0.5,0.1
#define TracerGColor 0.7,0.7,0.5
#define TracerNColor 0,0,0 // used for sniper / silenced rifles

#define TracerEColorF {TracerEColor,0.040}
#define TracerWColorF {TracerWColor,0.040}
#define TracerGColorF {TracerGColor,0.040}
#define TracerNColorF {TracerNColor,0.005}

#define TracerSEColorF {TracerEColor,0.25}
#define TracerSWColorF {TracerWColor,0.25}

/*!
\patch 1.50 Date 4/15/2002 by Ondra
- Fixed: Better visible bullet tracers for ZSU and Vulcan.
*/

#define TRACER_W_ALWAYS tracerColor[]=TracerWColorF;tracerColorR[]=TracerWColorF
#define TRACER_W_OPTIONAL tracerColor[]=TracerWColorF;tracerColorR[]=TracerNColorF
#define TRACER_W_STRONG tracerColor[]=TracerSWColorF;tracerColorR[]=TracerSWColorF

#define TRACER_E_ALWAYS tracerColor[]=TracerEColorF;tracerColorR[]=TracerEColorF
#define TRACER_E_OPTIONAL tracerColor[]=TracerEColorF;tracerColorR[]=TracerNColorF
#define TRACER_E_STRONG tracerColor[]=TracerSEColorF;tracerColorR[]=TracerSEColorF

#define TRACER_G_ALWAYS tracerColor[]=TracerGColorF;tracerColorR[]=TracerGColorF
#define TRACER_G_OPTIONAL tracerColor[]=TracerGColorF;tracerColorR[]=TracerNColorF

#define TRACER_N_ALWAYS tracerColor[]=TracerNColorF;tracerColorR[]=TracerNColorF

//lockable weapons
#define LockNo		0
#define LockCadet	1
#define LockYes		2

// destruction types
enum
{
  DestructNo,
  DestructBuilding,
  DestructEngine,
  DestructTree,
  DestructTent,
  DestructMan,
  DestructDefault,
  DestructWreck
};

//lock acquiring
#define manualLA      0
#define automaticLA   1

//lockable target type
#define AmmoLockGroundTT      0
#define AmmoLockAirGroundTT   1
#define AmmoLockAirTT         2

//missile lock type
#define fireAndForgetLT   0
#define keepLockedLT      1
#define manualLT          2

#define CanSeeRadar 1
#define CanSeeEye 2
#define CanSeeOptics 4
#define CanSeeEar 8
#define CanSeeCompass 16
#define CanSeeRadarC CanSeeRadar+CanSeeCompass
#define CanSeePeripheral 32
#define CanSeeAll 31

#define showAllNo		  0

/*!
	\patch 1.30 Date 11/02/2001 by Ondra
	- Fixed: Soldier was too long considered easily visible after firing.
*/

class CfgWeaponCursors
{
	class RifleCursorCore
	{
		texture = "";
		fade = 0.19; // alpha for max. inaccuracy
		class Sections
		{
			class Section1
			{
				uMin = 0; // area definition
				vMin = 0;
				uMax = 0.328125;
				vMax = 1;
				xOffset = -20; // offset definition (pixels for max. movement)
				yOffset = 0;
			};
			class Section2
			{
				uMin = 0.65625; // area definition
				vMin = 0;
				uMax = 1;
				vMax = 1;
				xOffset = +20; // offset definition (pixels for max. movement)
				yOffset = 0;
			};
		};
	};
	class MGCursorCore
	{
		texture = "";
		fade = 0.19; // alpha for max. inaccuracy
		class Sections
		{
			class Section1
			{
				uMin = 0; // area definition
				uMax = 0.328125;
				vMin = 0.328125;
				vMax = 0.65625;
				xOffset = -25; // offset definition (pixels for max. movement)
				yOffset = 0;
			};
			class Section2
			{
				uMin = 0.65625; // area definition
				uMax = 1;
				vMin = 0.328125;
				vMax = 0.65625;
				xOffset = +25; // offset definition (pixels for max. movement)
				yOffset = 0;
			};
			class Section3
			{
				uMin = 0.328125; // area definition
				uMax = 0.65625;
				vMin = 0.65625;
				vMax = 1;
				xOffset = 0; // offset definition (pixels for max. movement)
				yOffset = +25;
			};
		};
	};
	class LawCursorCore
	{
		texture = "";
		fade = 0.19; // alpha for max. inaccuracy
		class Sections
		{
			class Section1
			{
				uMin = 0; // area definition
				uMax = 0.328125;
				vMin = 0.328125;
				vMax = 0.65625;
				xOffset = -30; // offset definition (pixels for max. movement)
				yOffset = 0;
			};
			class Section2
			{
				uMin = 0.65625; // area definition
				uMax = 1;
				vMin = 0.328125;
				vMax = 0.65625;
				xOffset = +30; // offset definition (pixels for max. movement)
				yOffset = 0;
			};
			class Section3
			{
				uMin = 0.328125; // area definition
				uMax = 0.65625;
				vMin = 0;
				vMax = 0.328125;
				xOffset = 0; // offset definition (pixels for max. movement)
				yOffset = -30;
			};
			class Section4
			{
				uMin = 0.328125; // area definition
				uMax = 0.65625;
				vMin = 0.65625;
				vMax = 1;
				xOffset = 0; // offset definition (pixels for max. movement)
				yOffset = +30;
			};
		};
	};
};


class CfgAmmo
{
	access = ReadAndCreate;
	class DefaultAmmo
	{
		access = ReadOnlyVerified;

		hit=0;indirectHit=0;indirectHitRange=1;
		typicalSpeed=900;
		targetCategory="";
		
		model= "";
		decalClass = "";
		proxyShape = "";
		simulation = "";
		animated=false; // almost no ammo model are animated
		shadow=false; // ammo shadows disabled by default
		cost=0;
		maxSpeed=0;
		cartridge="";
		simulationStep=0.05;
		tracerColor[]=TracerGColorF; // default: yellow tracer
		tracerColorR[]=TracerGColorF; // default realistic: yellow tracer

		soundFly[]={,1,1};
		soundEngine[]={,1,1};
		soundHit[]={,1,1};
		// supersonic crack sound is played only when shot is traveling faster than speed of sound
		// near is used for 0..10 m, far for 50+m, cross-blending is done in between
		supersonicCrackNear[]={"",1,1};
		supersonicCrackFar[]={"",1,1};
		// list of sounds with probabilities

    hitGroundSoft[]={soundHit,1};
    hitGroundHard[]={soundHit,1};
		hitMan[]={soundHit,1};
		hitArmor[]={soundHit,1};
    hitIron[]={soundHit,1};
		hitBuilding[]={soundHit,1};
    hitFoliage[]={soundHit,1};
    hitWood[]={soundHit,1};
    hitGlass[]={soundHit,1};
    hitGlassArmored[]={soundHit,1};
    hitConcrete[]={soundHit,1};
    hitRubber[]={soundHit,1};
    hitPlastic[]={soundHit,1};
    hitDefault[]={soundHit,1};
    hitMetal[]={soundHit,1};
    hitMetalplate[]={soundHit,1};

		// max. angle at which the bullet will be deflected - 0 means no deflection
		deflecting=0;
    /// slowdown coefficient for deflection (speed will be multiplied by this coef after the ammo deflects from some surface)
    deflectionSlowDown=0.8;
		explosive=1;
		/// name of config class describing the explosion effects ("" for none);
		explosionEffects = "ExplosionEffects";
		/// name of config class describing the crater effects ("" for none);
		craterEffects = "ImpactEffectsBig";
		/// name of config class describing the tak na smoke trails ("" for none);
		effectsMissile = "ExplosionEffects";
		/// name of config class describing the smokeshell effects ("" for none);
		effectsSmoke = "SmokeShellWhite";
    /// effects from muzzle (for missiles, ShotShell, ShotBullet)
    effectsFire = "CannonFire";

		// unless overridden, the ammo in annihilated if it does not hit anything soon
		timeToLive=10;

		irLock=false;
		airLock= AmmoLockGroundTT; // 0 - only ground, 1 ground and air, 2 only air 
		laserLock=false;
    nvLock = false;
    artilleryLock = false;
		manualControl=false;
		maxControlRange=350; // max range for manual control
		maneuvrability=1.0; // missile control maneuvrability
		/// how much of the tracking is done using computed leading of the target (more accurate)
		trackLead=1.0; //Range 0-1 1 = perfect movement lead
		/// how much the missile oversteers (realistic - really used in simple missiles)
		trackOversteer=1.0; //1 = no oversteer

    //defines proxi ID offset for missiles (proxy index == ammo count + maverickweaponIndexOffset)
    maverickweaponIndexOffset = 0;

		sideAirFriction=1;

    //multiplier of artillery dispersion, when 1, there is  1 meter dispersion with every second of flight
    artilleryDispersion = 1;
    artilleryCharge = 1;

		visibleFire=0; // how much is visible when this weapon is fired
		audibleFire=0;
		visibleFireTime=0; // how long is it visible

		explosionTime=0; // by default there is no timer
		fuseDistance=0; // by default anything can explode immediately
		initTime=0;
		thrustTime=1.5;
		thrust=210;
		/*
		default thrust calculation was: maxSpeed*0.7;
		due to bug it was increased to maxSpeed*0.7*thurstTime
		normal missile speed is about 200 m/s
		*/
		airFriction = -0.0005;

		class HitEffects
		{
			// There can be defined impacts effects for different materials.
			// Example:  Impact_Metal = "ImpactEffectsMetal";
			vehicle = "ImpactMetal";
			object = "ImpactConcrete";
		};

		//multiplier of default caliber, 1 mean 7,62
		//used for kinetic power
		caliber = 1;
    whistleDist = 0; // value > 0 enable whistling, distance from source
    whistleOnFire = 0; // // 0 when fired, 1 when explode, 2 - both
		class NVGMarkers{};

	};
	class MissileCore : DefaultAmmo
	{
		simulation=shotMissile;
		simulationStep=0.05;
		timeToLive=20;

		manualControl=true;
		maxControlRange=250; // max range for manual control

		initTime=0.15;
		thrustTime=2.5;
		thrust=350;
		maneuvrability=3.0;

		visibleFire=32; // how much is visible when this weapon is fired
		audibleFire=32;
		visibleFireTime=20; // how long is it visible

		soundHit[]={,db40,1};
		soundFly[]={,db-40,2};
		soundEngine[]={,db-60,1};

		cost=10000;
		deflecting=5;
	};
	/// free fall bomb
	class BombCore: DefaultAmmo
	{
		hit=50;indirectHit=40;indirectHitRange=10;

		cost=200;

		irLock=false;
		laserLock=false;
		maxControlRange=0; // no control (passive weapon)
		maneuvrability=16.0;
		sideAirFriction=0.1;
		simulation=shotMissile;
		maxSpeed=100;

		timeToLive=120;
		initTime=0;
		thrustTime=0;
		thrust=0;
	};
	/// laser guided bomb
	class LaserBombCore: BombCore
	{
		hit=5000;indirectHit=3500;indirectHitRange=15;
		cost=20000;
		irLock=false;
		laserLock=true;
		maxControlRange=100000; // unlimited control (active weapon)
		maneuvrability=16.0;
		sideAirFriction=0.1;
		simulation=shotMissile;
		maxSpeed=100;
		timeToLive=120;

		initTime=0;
		thrustTime=0;
		thrust=0;
	};

	class RocketCore : DefaultAmmo
	{
		simulation=shotRocket;
		simulationStep=0.05;
		timeToLive=20;

		maneuvrability=0.0;
		maxControlRange=0;
		initTime=0.05;
		thrustTime=1;
		thrust=1500;

		visibleFire=32;
		audibleFire=32;
		visibleFireTime=20;

		cost=2000;
		deflecting=5;
	};

  class ArtilleryRocketCore : RocketCore
  {
    artilleryLock = true;
  };

	class BulletCore : DefaultAmmo
	{
		simulation=shotBullet;
		simulationStep=0.05;
		timeToLive=3; // how long does the fired object live before annihilating

		soundFly[]={,db-12,0.7};

		explosive=0;

		visibleFire=16;
		audibleFire=16;
		visibleFireTime=3;

		soundHit1[]={,db-30,1};
		soundHit2[]={,db-30,1};
		soundHit3[]={,db-30,1};
		soundHitMan1[]={,db-38,1};
		soundHitMan2[]={,db-38,1};
		soundHitArmor1[]={,db-30,1};
		soundHitArmor2[]={,db-30,1};
		soundHitBuilding1[]={,db-30,1};
		soundHitBuilding2[]={,db-30,1};
		hitGround[]={soundHit1,0.33,soundHit2,0.33,soundHit3,0.33};
		hitMan[]={soundHitMan1,0.5,soundHitMan2,0.5};
		hitArmor[]={soundHitArmor1,0.7, soundHitArmor2, 0.3};
		hitBuilding[]={soundHitBuilding1,0.5,soundHitBuilding2,0.5};

		cost = 0.7;
	};
  class ShotgunCore: DefaultAmmo
  {
    simulation = shotSpread;
    simulationStep=0.05;
    timeToLive=3; // how long does the fired object live before annihilating

    soundFly[]={,db-12,0.7};

    explosive=0;

    visibleFire=16;
    audibleFire=16;
    visibleFireTime=3;

    soundHit1[]={,db-30,1};
    soundHit2[]={,db-30,1};
    soundHit3[]={,db-30,1};
    soundHitMan1[]={,db-38,1};
    soundHitMan2[]={,db-38,1};
    soundHitArmor1[]={,db-30,1};
    soundHitArmor2[]={,db-30,1};
    soundHitBuilding1[]={,db-30,1};
    soundHitBuilding2[]={,db-30,1};
    hitGround[]={soundHit1,0.33,soundHit2,0.33,soundHit3,0.33};
    hitMan[]={soundHitMan1,0.5,soundHitMan2,0.5};
    hitArmor[]={soundHitArmor1,0.7, soundHitArmor2, 0.3};
    hitBuilding[]={soundHitBuilding1,0.5,soundHitBuilding2,0.5};

    cost = 0.7;
  };
	class ShellCore : DefaultAmmo
	{
		simulation=shotShell;
		simulationStep=0.05;
		timeToLive=20; // how long does the fired object live before annihilating
		cost=1000;
		soundHit[]={,db50,1};
		soundFly[]={,db-30,4};

		indirectHitRange=8;
		visibleFire=16; // how much is visible when this weapon is fired
		audibleFire=16;
		visibleFireTime=10; // how long is it visible
		deflecting=5;
	};
	class GrenadeCore: DefaultAmmo
	{
		explosive=1;
		hit=20;indirectHit=18;indirectHitRange=7;
		cost=40;
		simulation=shotShell;
		simulationStep=0.05;
		soundHit[]={,db20,1};
		soundFly[]={,db-90,1};
		soundEngine[]={,db-80,4};

		visibleFire=2; // how much is visible when this weapon is fired
		audibleFire=0.25;
		visibleFireTime=0; // how long is it visible
		timeToLive=20; // how long does the fired object live before annihilating
		deflecting=60;
	};
	class TimeBombCore : DefaultAmmo
	{
		hit=700;indirectHit=700;indirectHitRange=7;
		model="";
		cost=2000;
		simulation=shotTimeBomb;
		simulationStep=0.1;
		soundHit[]={,db40,1};
		soundFly[]={,0,1};
		soundEngine[]={,0,1};

		visibleFire=0;
		audibleFire=0;
		visibleFireTime=0;
		timeToLive=0; // unlimited
	};
	class PipeBombCore : TimeBombCore
	{
		simulation=shotPipeBomb;
	};
	class MineCore : TimeBombCore
	{
		hit=750;indirectHit=750;indirectHitRange=5;
		simulation=shotMine;
		model = "";
		defaultMagazine = "";
	};
	class FlareCore : GrenadeCore
	{
		timeToLive=30; // how long does the fired object live before annihilating

		hit=5;indirectHit=2;indirectHitRange=0.2;
		cost=100;
		simulation=shotIlluminating;
		simulationStep=0.05;
		explosive = 0;
		soundHit[]={,0,1};
		lightColor[] = {1, 1, 1, 0};
    brightness = 2;
    size = 1;
		deflecting=60;
	};
	class SmokeShellCore : GrenadeCore
	{
		timeToLive=60; // how long does the fired object live before annihilating
		visibleFire=2; // how much is visible when this weapon is fired
		audibleFire=0.25;
		visibleFireTime=1;
		hit=5;indirectHit=2;indirectHitRange=0.2;
		cost=100;

		simulation=shotSmoke;
		explosive = 0;
		soundHit[]={,0,1};
		smokeColor[] = {1, 1, 1, 0};

		/*
		//obsolete, now it is in effectsSmoke
		class Smoke
		{
			interval = 0.01;
			// single cloudlet parameters
			cloudletDuration = 4.0; // time to live (not including fades)
			cloudletAnimPeriod = 1.0; // animation speed
			cloudletSize = 0.25;
			cloudletAlpha = 1.0;
			cloudletGrowUp = 1.5; // grow-up time
			cloudletFadeIn = 0.0; // fade-in time
			cloudletFadeOut = 5.0; // fade-out time
			cloudletAccY = -0.3; // vertical acceleration
			cloudletMinYSpeed = 0; // vertical speed range
			cloudletMaxYSpeed = 10;
			cloudletShape = "cl_basic";
			cloudletColor[] = {1, 1, 1, 0};
			initT = 1000;
			deltaT = -500;
			class Table
			{
				class T1 {maxT = 0; color[] = {0.7, 0.7, 0.7, 1};}
				class T2 {maxT = 1000; color[] = {0.4, 0.4, 0.4, 1};}
			};
			// source parameters
			density = 1.0;
			size = 1.5;
			initYSpeed = 1.0;
			timeToLive = 1e20; // time to live (not including fades)
			in = 3.0; // fade-in time
			out = 0.0; // fade-out time
		};*/
		deflecting=60;
	};
	//Used to give a fake threatlevel to a unit
	class FakeAmmo : BulletCore
	{
		hit=13;indirectHit=4;indirectHitRange=0.2;
		cost=6;
		airlock = AmmoLockAirGroundTT;
		explosive=0;

		visibleFire=16; // how much is visible when this weapon is fired
		audibleFire=16;
		visibleFireTime=3; // how long is it visible
	};
};

class CfgRecoils
{
	access = ReadAndCreate;

	empty[]={};
	// time, offset, angle
	impulse[]=
	{
		0.03,0.02,0.2,
		0.08,0.08,0.8,
		0.10,0.03,-0.1,
		0.30,0,0
	};
};
class CfgMagazines
{
	class DefaultMagazine
	{
		scope= 0;
		value = 2;
		type = WeaponNoSlot;
		simulation = ProxyMagazines; //Might be needed for count and other magazine specific values.
    inventorySlot = ""; 
    attachments[] = {};
    handheld = false; 
    rotateDropped = true;
	rotateAlways = true;
	rotateY = false;
	rotateZ = false;
    autoQuickbar = false;
    cargoClass = ""; //only this kind of item can be stored

    itemSize[] = {1, 1};
    targetCategory="";

    storeVariables[] = {};
    synchronizedVariables[] = {};

    armAction="Rifle";

    model="";
		picture="";
		ammo="";
		count=1;
		displayName="";
		displayNameShort=""; 
		nameSound="";
		weight = 0;
		maxLeadSpeed=50; // max estimated speed km/h
		initSpeed = 100;
		reloadAction = "";
		useAction = false;
		useActionTitle = "";
		modelSpecial = "";
		class InventoryPlacements {};
		class Library
		{
			libTextDesc = "";
		};
		descriptionShort="";
    /// max. time of holding throw button
    maxThrowHoldTime = 2.0;
    /// min. coef for throwing (to multiply initSpeed)
    minThrowIntensityCoef = 0.3;
    /// max. coef for throwing (to multiply initSpeed)
    maxThrowIntensityCoef = 1.4; 
    /// when magazine is changed, weapon is ready to fire
    quickReload = false;

    /************************************************************************************************/
    // Inherited for EntityAIType 
    // TODO: try to reduce

    reversed = false; // magazines / proxies are not reversed

    speechSingular[] = {};
    speechPlural[] = {};
    textSingular = "";
    textPlural = "";

    weaponSlots = 0;

    clothingType = "";

    tooltip = "";
    quantity = "format ['%1/%2', magazineAmmo _this, maxMagazineAmmo _this]";

    camouflage=2; // how difficult to spot - bigger - better spotable
    audible=1; // audible - bigger - better heard
    accuracy=0; // accuracy needed to recognize type of this target

    sensitivity=2.5; // sensor sensitivity
    sensitivityEar=0.0075;

    spotableDarkNightLightsOff=0.003; // 0.02 was too much, soldier was seen in a dark night
    spotableNightLightsOff=0.035;
    spotableNightLightsOn=4;

    visibleNightLightsOff=0.1;
    visibleNightLightsOn=0.2;

    side=TSideUnknown;
    destrType=DestructDefault;

    portrait=; // Picture in Command Bar
    icon = iconObject;
    unitInfoType=RscUnitInfoTank;
    hideUnitInfo=false;

    cost=500000;
    fuelCapacity=0;
    fuelConsumptionRate = 0.01f;

    armor=100;
    armorLights=0.4;
    damageResistance = 0.004;

    secondaryExplosion=-1;

    minFireTime=20; // minimal time spent firing on single target

    extCameraPosition[]={0,2,-20};
    groupCameraPosition[]={0,5,-30};

    maxSpeed=80; // max speed on level road, km/h
    limitedSpeedCoef = 0.22;
    brakeDistance=5; // vehicle movement precision
    precision=5; // vehicle movement precision
    formationX=10; // formation spacing
    formationZ=20;
    formationTime=5;

    steerAheadSimul=0.3; // steer point on path properties
    steerAheadPlan=0.4;
    predictTurnSimul=1.2; // brake before turns
    predictTurnPlan=1.0;

    driverCanEject = true;
    cargoCanEject = true;

    enableWatch = false; // watch device is not present by default
    enableRadio = false; // radio station is not present by default
    enableGPS = false; // default vehicle has not GPS miniMap enabled

    transportFuel = 0;
    transportRepair = 0;
    transportAmmo = 0;
    class TransportWeapons {};
    class TransportMagazines {};

    // no cargo space for magazines
    itemsCargoSize[] = {0, 0}; 

    attendant = false;
    engineer = false;

    lockDetectionSystem = 0;
    incommingMisslieDetectionSystem = 0;

    alwaysTarget = false;
    irTarget = true;
    irScanRangeMin=0;
    irScanRangeMax=0;
    irScanToEyeFactor=1;
    irScanGround=true;

    laserTarget = false;
    laserScanner = false;
    nvTarget = false;
    nvScanner = false;
    artilleryTarget = false;
    artilleryScanner = false;

    nightVision=false;

    commanderCanSee = CanSeeAll;
    gunnerCanSee = CanSeeOptics+CanSeeEar+CanSeeCompass;
    driverCanSee = CanSeeEye+CanSeeEar+CanSeeCompass;
    radarType = 0;

    allowTabLock = true;
    showAllTargets = showAllNo;

    HeadAimDown = 0;

    preferRoads=false; // never used value, engine don't use it

    soundEngine[] = {"", 1, 1};
    soundEnviron[] = {"", 1, 1};
    soundCrash[] = {"", db-10, 1};
    soundLandCrash[] = {"", db-0, 1};
    soundWaterCrash[] = {"", db-15, 1};
    soundGetIn[] = {"", db-70, 1};
    soundGetOut[] = {"", db-70, 1};
    soundServo[] = {"", db-50, 0.5};
    soundDammage[] = {"", 1, 1};
    soundEngineOnInt[] = {"", 1, 1};
    soundEngineOffInt[] = {"", 1, 1};
    soundEngineOnExt[] = {"", 1, 1};
    soundEngineOffExt[] = {"", 1, 1};
    soundLocked[] = {"", 1, 1};
    soundIncommingMissile[] = {"", 1, 1};
    class SoundEnvironExt {};
    class SoundEquipment {};
    class SoundGear {};

    weapons[]={};
		magazines[] = {};
    magazineSlot = "";

    threat[]={0.7, 0.5, 0.3};

    class HeadLimits
    {
      initAngleX=5; minAngleX=-40; maxAngleX=+40;
      initAngleY=0; minAngleY=-90; maxAngleY=+90;
      minAngleZ=-45; maxAngleZ=+45; 
      rotZRadius=0.2; //distance from neck to eye to rotate camera with shifted center of rotation
    };
    class ViewPilot
    {
      initAngleX=5; minAngleX=-85; maxAngleX=+85; //Rg 30;
      initAngleY=0; minAngleY=-150; maxAngleY=+150; //Rg 100;
      initFov=0.9; minFov=0.42; maxFov=0.9;
      minMoveX=0; maxMoveX=0; minMoveY=0; maxMoveY=0; minMoveZ=0; maxMoveZ=0;
    };
    class PilotSpec
    {
      showHeadPhones = false;
    };

    selectionClan = "";
    selectionDashboard = "";
    selectionShowDamage = "";
    selectionBackLights = "";

    memoryPointSupply = "doplnovani";

    class Reflectors {};
    aggregateReflectors[] = {};

    class MarkerLights{};
    class NVGMarkers {};

    // selections, where texture can be redefined
    hiddenSelections[] = {};
    // initial texture in hidden selections
    hiddenSelectionsTextures[] = {};

    /// selection used for total damage (parts of the model connected to no hitzone)
    selectionDamage="zbytek";
  };
	class FakeMagazine: DefaultMagazine
	{
		scope= 1;
		ammo = FakeAmmo;
    selectionFireAnim = "zasleh";
    inventorySlot = "magazineFakeWeapon"; 
	};
};
//Weapon firing modes
class Mode_SemiAuto
{
	multiplier = 1;
	burst = 1;
	displayName = "";
	dispersion = 0.0002;
	sound[] = {, db+20, 1};
  soundBegin[] = {sound, 1}; // diff variants for single shot
  soundEnd[] = {};
  soundLoop[] = {};
	soundContinuous = 0;
	soundBurst = true;
	reloadTime = 0.100000;
	ffCount = 1;
	ffMagnitude = 0.5;
	ffFrequency = 11;
	flash = gunfire;
	flashSize = 0.1;
	recoil = Empty;
	recoilProne = Empty;
	autoFire = 0;
	aiRateOfFire = 0.500000;
	aiRateOfFireDistance = 500;
	useAction = 0;
	useActionTitle = "";
	showToPlayer = true;
	minRange=30;minRangeProbab=0.25;
	midRange=300;midRangeProbab=0.58;
	maxRange=600;maxRangeProbab=0.04;
  artilleryDispersion = 1;
  artilleryCharge = 1;
};
class Mode_Burst: Mode_SemiAuto
{
	displayName = "";
	sound[] = {,db+20,1};
  // shot for burst are split into 3 parts:
  // soundBegin = first shot, soundLoop = [2, burst - 1], soundEnd = last shot + echo
  soundLoop[] = {sound, 1};
  soundEnd[] = {sound, 1};
	soundBurst = true;
	burst=3;
	dispersion = 0.0005;
	minRange=10;minRangeProbab=0.30;
	midRange=60;midRangeProbab=0.58;
	maxRange=150;maxRangeProbab=0.04;
};
class Mode_FullAuto: Mode_SemiAuto
{
	displayName = "";
	dispersion = 0.0005;
	sound[] = {,db+20,1};
  soundEnd[] = {sound, 1};
	soundContinuous = 0;
	reloadTime = 0.080000;
	autoFire = true;
	minRange=1;minRangeProbab=0.20;
	midRange=30;midRangeProbab=0.58;
	maxRange=80;maxRangeProbab=0.04;
};

// weapons
class CfgWeapons
{
	//-->Weapons and magazines
	access = ReadAndCreate;

	// define weapons
	// weapon: ammo name, max count, reload time
	class DefaultWeapon
	{
		access = ReadOnlyVerified;

		scope = private;

		value = 2;

		picture=""; // default picture name is class name
		uiPicture=;
		ammo= "";
		cursor = "";
		cursorAim = "";
		// relative cursor size, 1 is normal
		cursorSize = 1.0;
		showAimCursorInternal = true; // aim cursor is shown in internal view
		cursorAimOn = ""; // cursor used when laser is on
		laser = false; // laser designator

    simulation = "Weapon"; // default: regular weapon (special items such a Binocular, Radio, GPS etc. have different simulation)
    inventorySlot = ""; 
    attachments[] = {};
    handheld = true; 
    rotateDropped = false;
	rotateY = false;
	rotateZ = false;
    autoQuickbar = true;	//automatically add item to quickbar
    cargoClass = ""; //only this kind of item can be stored
    itemSize[] = {1, 1};
    targetCategory="";

    storeVariables[] = {};
    synchronizedVariables[] = {};

    armAction="Rifle";

    tooltip = "";
    quantity = "";

    type=WeaponHardMounted; // default: hard mounted
		displayName = "";
		nameSound="";
		count=0;
		multiplier=1; // convert from internal to display
		burst=1;
		magazineReloadTime=0;
		reloadTime=1.0;
		sound[]={,1,1};
    soundBegin[]={sound, 1};
    soundEnd[]={sound, 1};
    soundLoop[]={sound, 1};
		soundContinuous=false;
    /// is the sound for the whole burst (true) or for individual bullets (false)
    /** Note: When there is no burst (burst=1), the value has no effect */
    soundBurst=true;
		drySound[]={,1,1};
		reloadSound[]={,1,1};
		reloadMagazineSound[]={,1,1};
    emptySound[]={"",1,1};
    soundBullet[]={emptySound, 1};
		initSpeed=0;
		ballisticsComputer = 0;
    irDistance = 0;//infrared laser beam, 0 - no IR

		dispersion=0.002;
		aiDispersionCoefX=1.0;
		aiDispersionCoefY=1.0;
		canLock=LockYes;
    lockAcquire = automaticLA;
		enableAttack=true;
		ffMagnitude=0;
		ffFrequency=1;
		ffCount=1;
		recoil= "empty";
		// recoil prone is used when animation aimPrecision is lower than 0.4 (prone is usualy 0.3)
		// when empty, using the same recoil when prone, only modified using animation aimPrecision
		recoilProne= "";
		model="";
		modelSpecial="";
		modelMagazine="";
		// position and direction of muzzle
		muzzlePos="usti hlavne";
		muzzleEnd="konec hlavne";
    // position and direction of ir laser
    irLaserPos="laser pos";
    irLaserEnd="laser dir";
		// position and velocity of outgoing empty cartridge
		cartridgePos="nabojnicestart";
		cartridgeVel="nabojniceend";

		selectionFireAnim = "zasleh";
		memoryPointCamera = "eye";

    //multiple optics modes
    useModelOptics = true; //use model if present (one is used for all modes)
    opticsID = 0; //optics ID - allows to disable for some fire mod 

		modelOptics="";
    opticsPPEffects[] = {};
		opticsFlare = true;
		optics = true;
    // 0 = never force optics
    // 1 = force optics both in internal and external view
    // 2 = force optics only in internal view, external need not use them
		forceOptics = 0;

    // some weapons need to use binocular animations (currently works for laser designators only)
    useAsBinocular = false;
		// optics normally reduce peripheral vision, but keep it to some extent
		opticsDisablePeripherialVision = 0.67;

		opticsZoomMin=0.35;
		opticsZoomMax=0.35;
		opticsZoomInit=0.35;
		distanceZoomMin=400;
		distanceZoomMax=400;

		primary = 10;
		showSwitchAction = false;
		showEmpty = true;
		autoFire = false;
		autoReload = true;
		autoAimEnabled=true;

		showToPlayer = true;

		aiRateOfFire=5.0; // delay between shots at given distance
		aiRateOfFireDistance=500; // at shorter distance delay goes lineary to zero

    fireLightDuration = 0.05; // duration of light flash
    fireLightIntensity = 0.012; // intensity of light flash

		backgroundReload = false;
		reloadAction = "";
		ejectType = 1; //1 = default, 2 = on reload, 4+ ... future implementation

		// class is used for weapon, muzzle, magazine and mode
		// weapon level
		muzzles[] = {this};		// muzzles on weapon
		// muzzle level
		magazines[] = {};	// available magazines for muzzle
		// magazine level
		modes[] = {this};			// modes of magazine (muzzle)
		// mode level

		useAction = false;
		useActionTitle = "";

		canDrop = true;

    weight = 0;

		minRange=1;minRangeProbab=0.30;
		midRange=150;midRangeProbab=0.58;
		maxRange=500;maxRangeProbab=0.04;

                // animations files for Inverse Kinematics (Hands)
                // handAnim[]={skeletonname, rtmfile,skeletonname2, rtmfile2,...};
                handAnim[]={};

    artilleryDispersion = 1;
    artilleryCharge = 1;

    // shot spread for shotguns (SpreadShot fires)
    fireSpreadAngle = 1.25f;

		class Library
		{
			libTextDesc = "";
		};
		descriptionShort="";

    /************************************************************************************************/
    // Inherited for EntityAIType 
    // TODO: try to reduce

    reversed = false; // weapons / proxies are not reversed

    speechSingular[] = {};
    speechPlural[] = {};
    textSingular = "";
    textPlural = "";

    weaponSlots = 0;

    clothingType = "";

    camouflage=2; // how difficult to spot - bigger - better spotable
    audible=1; // audible - bigger - better heard
    accuracy=0; // accuracy needed to recognize type of this target

    sensitivity=2.5; // sensor sensitivity
    sensitivityEar=0.0075;

    spotableDarkNightLightsOff=0.003; // 0.02 was too much, soldier was seen in a dark night
    spotableNightLightsOff=0.035;
    spotableNightLightsOn=4;

    visibleNightLightsOff=0.1;
    visibleNightLightsOn=0.2;

    side=TSideUnknown;
    destrType=DestructDefault;

    portrait=; // Picture in Command Bar
    icon = iconObject;
    unitInfoType=RscUnitInfoTank;
    hideUnitInfo=false;

    cost=500000;
    fuelCapacity=0;
    fuelConsumptionRate = 0.01f;

    armor=100;
    armorLights=0.4;
    damageResistance = 0.004;

    secondaryExplosion=-1;

    minFireTime=20; // minimal time spent firing on single target

    extCameraPosition[]={0,2,-20};
    groupCameraPosition[]={0,5,-30};

    maxSpeed=80; // max speed on level road, km/h
    limitedSpeedCoef = 0.22;
    brakeDistance=5; // vehicle movement precision
    precision=5; // vehicle movement precision
    formationX=10; // formation spacing
    formationZ=20;
    formationTime=5;

    steerAheadSimul=0.3; // steer point on path properties
    steerAheadPlan=0.4;
    predictTurnSimul=1.2; // brake before turns
    predictTurnPlan=1.0;

    driverCanEject = true;
    cargoCanEject = true;

    enableWatch = false; // watch device is not present by default
    enableRadio = false; // radio station is not present by default
    enableGPS = false; // default vehicle has not GPS miniMap enabled

    transportFuel = 0;
    transportRepair = 0;
    transportAmmo = 0;
    class TransportWeapons {};
    class TransportMagazines {};

    // no cargo space for weapons
    itemsCargoSize[] = {0, 0}; 

    attendant = false;
    engineer = false;

    lockDetectionSystem = 0;
    incommingMisslieDetectionSystem = 0;

    alwaysTarget = false;
    irTarget = true;
    irScanRangeMin=0;
    irScanRangeMax=0;
    irScanToEyeFactor=1;
    irScanGround=true;

    laserTarget = false;
    laserScanner = false;
    nvTarget = false;
    nvScanner = false;
    artilleryTarget = false;
    artilleryScanner = false;

    nightVision=false;

    commanderCanSee = CanSeeAll;
    gunnerCanSee = CanSeeOptics+CanSeeEar+CanSeeCompass;
    driverCanSee = CanSeeEye+CanSeeEar+CanSeeCompass;
    radarType = 0;

    allowTabLock = true;
    showAllTargets = showAllNo;

    HeadAimDown = 0;

    preferRoads=false; // never used value, engine don't use it

    soundEngine[] = {"", 1, 1};
    soundEnviron[] = {"", 1, 1};
    soundCrash[] = {"", db-10, 1};
    soundLandCrash[] = {"", db-0, 1};
    soundWaterCrash[] = {"", db-15, 1};
    soundGetIn[] = {"", db-70, 1};
    soundGetOut[] = {"", db-70, 1};
    soundServo[] = {"", db-50, 0.5};
    soundDammage[] = {"", 1, 1};
    soundEngineOnInt[] = {"", 1, 1};
    soundEngineOffInt[] = {"", 1, 1};
    soundEngineOnExt[] = {"", 1, 1};
    soundEngineOffExt[] = {"", 1, 1};
    soundLocked[] = {"", 1, 1};
    soundIncommingMissile[] = {"", 1, 1};
    class SoundEnvironExt {};
    class SoundEquipment {};
    class SoundGear {};

    weapons[]={};

    threat[]={0.7, 0.5, 0.3};

    class HeadLimits
    {
      initAngleX=5; minAngleX=-40; maxAngleX=+40;
      initAngleY=0; minAngleY=-90; maxAngleY=+90;
      minAngleZ=-45; maxAngleZ=+45; 
      rotZRadius=0.2; //distance from neck to eye to rotate camera with shifted center of rotation
    };
    class ViewPilot
    {
      initAngleX=5; minAngleX=-85; maxAngleX=+85; //Rg 30;
      initAngleY=0; minAngleY=-150; maxAngleY=+150; //Rg 100;
      initFov=0.9; minFov=0.42; maxFov=0.9;
      minMoveX=0; maxMoveX=0; minMoveY=0; maxMoveY=0; minMoveZ=0; maxMoveZ=0;
    };
    class PilotSpec
    {
      showHeadPhones = false;
    };

    selectionClan = "";
    selectionDashboard = "";
    selectionShowDamage = "";
    selectionBackLights = "";

    memoryPointSupply = "doplnovani";

    class Reflectors {};
    aggregateReflectors[] = {};

    class MarkerLights{};
    class NVGMarkers {};

    // selections, where texture can be redefined
    hiddenSelections[] = {};
    // initial texture in hidden selections
    hiddenSelectionsTextures[] = {};
	hiddenSelectionsMaterials[] = {};

    /// selection used for total damage (parts of the model connected to no hitzone)
    selectionDamage="zbytek";
  };
/*	class MagazineHandGun: Weapon
	{
		magazineType = WeaponSlotHandGunItem;
	};
	class MagazineItem: Weapon
	{
		magazineType = WeaponSlotItem;
	};
	class Magazine2Item: Weapon
	{
		magazineType = 2*WeaponSlotItem;
	};
*/
	class PistolCore: DefaultWeapon
	{
		dexterity = 2; //Ingame weapon handling value, lower value = takes more time to traverse a weapon
		type = WeaponSlotHandGun;
		canLock=LockNo;
    // armAction="Handgun";
	};
	class RifleCore: DefaultWeapon
	{
		type = WeaponSlotPrimary;
		dexterity = 1.7;
		aiRateOfFire = 0.5;
		aiRateOfFireDistance = 500;
		canLock=LockNo;

    inventorySlot = Shoulder; 
    itemSize[] = {2, 1};
	};
	class MGunCore: DefaultWeapon
	{
		type = WeaponSlotPrimary+WeaponSlotSecondary;
		dexterity = 0.5;
		aiRateOfFire = 0.5;
		aiRateOfFireDistance = 400;

    inventorySlot = Shoulder; 
    itemSize[] = {2, 1};
  };
	class LauncherCore: DefaultWeapon
	{
		type = WeaponSlotSecondary;
		dexterity = 0.5;
		aiRateOfFire = 0.5;
		aiRateOfFireDistance = 300;
		count = 1;

    inventorySlot = Shoulder; 
    itemSize[] = {2, 1};
	};
	class GrenadeWeaponCore: DefaultWeapon
	{
		type = WeaponNoSlot;
		dexterity = 3;
		aiRateOfFire = 1;
		aiRateOfFireDistance = 40;
		count = 1;
		canLock=LockNo;
  };
	class CannonCore: DefaultWeapon
	{
		type = WeaponHardMounted;
	};
	class FakeWeapon : MGunCore
	{
		scope = protected;
		type=WeaponNoSlot;
		cursor =;
		cursoraim =;
		sound[]={};
		magazines[]= {FakeMagazine};
    magazineSlot = "magazineFakeWeapon"; 
	};
};