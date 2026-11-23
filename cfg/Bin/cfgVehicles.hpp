// vehicles and ammo definition

// show all targets
#define showAllNo		  0
#define showAllCadet	1
#define showAllYes		2


class CfgCloudlets
{
  access=ReadAndWrite;

  // Ancestor of all cloudlet classes
  class Default
  {
    particleFSNtieth = 1;
    particleFSIndex = 0;
    particleFSFrameCount = 1;
    particleFSLoop = 1;
    angle = 0;
    angleVar = 0;
  };

  class Explosion
  {
    access = ReadAndWrite;
    // single cloudlet parameters
    cloudletDuration = 0.2; // time to live (not including fades)
    cloudletAnimPeriod = 0.5; // animation speed
    cloudletSize = 1.0;
    cloudletAlpha = 1.0;
    cloudletGrowUp = 0.05; // grow-up time
    cloudletFadeIn = 0.01; // fade-in time
    cloudletFadeOut = 0.2; // fade-out time
    cloudletAccY = -0.2; // vertical acceleration
    cloudletMinYSpeed = 0; // vertical speed range
    cloudletMaxYSpeed = 6;
    cloudletShape = cloudletExplosion;
    cloudletColor[] = {1, 1, 1, 0};
    // source parameters
    interval = 0.001;
    size = 4.0; // particle size
    sourceSize = 3.0; // size of area from which particles are emitted
    timeToLive = 0;

    initT = 2500;
    deltaT = -4000;
    class Table
    {
      class T14 {maxT = 1400; color[] = {0.91, 0.5, 0.17, 0};}
      class T15 {maxT = 1500; color[] = {1, 0.6, 0.2, 0};}
      class T16 {maxT = 1600; color[] = {1, 0.71, 0.3, 0};}
      class T17 {maxT = 1700; color[] = {0.98, 0.83, 0.41, 0};}
      class T18 {maxT = 1800; color[] = {0.98, 0.91, 0.54, 0};}
      class T19 {maxT = 1900; color[] = {0.98, 0.99, 0.6, 0};}
      class T20 {maxT = 2000; color[] = {0.96, 0.99, 0.72, 0};}
      class T21 {maxT = 2100; color[] = {1, 0.98, 0.91, 0};}
      class T22 {maxT = 2200; color[] = {1, 1, 1, 0};}
    }
  };

#define CS_SPEED  0.8
  class CraterSmoke1
  {
    access = ReadAndWrite;
    interval = 0.05 / CS_SPEED;
    cloudletAnimPeriod = 1 * CS_SPEED;
    cloudletSize = 2.0;
    cloudletAlpha = 1.0; // not used
    cloudletGrowUp = 1.0 * CS_SPEED;
    cloudletFadeIn = 0.1 * CS_SPEED;
    cloudletDuration = 3.0 * CS_SPEED;
    cloudletFadeOut = 1.5 * CS_SPEED;
    cloudletAccY = -0.1;
    cloudletMinYSpeed = 0.0 / CS_SPEED;
    cloudletMaxYSpeed = 5.0 / CS_SPEED;
    cloudletColor[] = {1, 1, 1, 0};
    initT = 500;
    deltaT = -50 * CS_SPEED;
    class Table
    {
      class T0 {maxT = 0; color[] = {1, 1, 1, 0};}
      class T1 {maxT = 500; color[] = {0.5, 0.5, 0.5, 0};}
    }
    cloudletShape = cloudletSmoke;
    density = 1.0 / 0.7;    // used for alpha
    size = 1.0;         // source size
    timeToLive = 0.4 * CS_SPEED;
    in = 0.0 * CS_SPEED;
    out = 0.0 * CS_SPEED;
    initYSpeed = 5.0 / CS_SPEED;
  };
  class CraterSmoke2
  {
    access = ReadAndWrite;
    interval = 0.1 / CS_SPEED;
    cloudletAnimPeriod = 1 * CS_SPEED;
    cloudletSize = 2.0;
    cloudletAlpha = 1.0; // not used
    cloudletGrowUp = 1.0 * CS_SPEED;
    cloudletFadeIn = 0.1 * CS_SPEED;
    cloudletDuration = 2.5 * CS_SPEED;
    cloudletFadeOut = 1.5 * CS_SPEED;
    cloudletAccY = -0.1;
    cloudletMinYSpeed = 0.0 / CS_SPEED;
    cloudletMaxYSpeed = 2.0 / CS_SPEED;
    cloudletColor[] = {1, 1, 1, 0};
    initT = 0;
    deltaT = 0 * CS_SPEED;
    class Table
    {
      class T0 {maxT = 0; color[] = {1, 1, 1, 0};}
    }
    cloudletShape = cloudletSmoke;
    density = 1.0 / 0.7;    // used for alpha
    size = 1.0;         // source size
    timeToLive = 1.0 * CS_SPEED;
    in = 0.0 * CS_SPEED;
    out = 2.0 * CS_SPEED;
    initYSpeed = 0 / CS_SPEED;
  };
  class CraterSmoke3
  {
    access = ReadAndWrite;
    interval = 0.1 / CS_SPEED;
    cloudletAnimPeriod = 1 * CS_SPEED;
    cloudletSize = 1.0;
    cloudletAlpha = 0.3; // not used
    cloudletGrowUp = 1.0 * CS_SPEED;
    cloudletFadeIn = 0.5 * CS_SPEED;
    cloudletDuration = 3 * CS_SPEED;
    cloudletFadeOut = 1 * CS_SPEED;
    cloudletAccY = -0.1 * CS_SPEED;
    cloudletMinYSpeed = 0.0 / CS_SPEED;
    cloudletMaxYSpeed = 5.0 / CS_SPEED;
    cloudletColor[] = {1, 1, 1, 0};
    initT = 500;
    deltaT = -80 * CS_SPEED;
    class Table
    {
      class T0 {maxT = 0; color[] = {1, 1, 1, 0};}
    }
    cloudletShape = cloudletSmoke;
    density = 0.5 / 0.7;    // used for alpha
    size = 1.0;         // source size
    timeToLive = 2.0 * CS_SPEED;
    in = 0.2 * CS_SPEED;
    out = 0.0 * CS_SPEED;
    initYSpeed = 5.0 / CS_SPEED;
  };
  class CraterDustSmall
  {
    access = ReadAndWrite;
    interval = 0.01;
    cloudletSize = 1; // not used
    cloudletAlpha = 0.6;
    cloudletDuration = 0.2;
    cloudletAnimPeriod = 1;
    cloudletGrowUp = 0.6;
    cloudletFadeIn = 0;
    cloudletFadeOut = 0.6;
    cloudletAccY = -3.5;
    cloudletMinYSpeed = -20;
    cloudletMaxYSpeed = 10;
    cloudletColor[] = {0.25, 0.23, 0.16, 0};
    initT = 0;
    deltaT = 0;
    class Table
    {
      class T0 {maxT = 0; color[] = {1, 1, 1, 0};}
    }
    cloudletShape = cloudletDust;
    size = 0.2;
    sourceSize = 0.02;
  };
  class CraterBlood : CraterDustSmall
  {
    access = ReadAndWrite;
    interval = 0.005;
    cloudletAlpha = 0.5;
    cloudletDuration = 0.1;
    cloudletAnimPeriod = 1;
    cloudletGrowUp = 0.3;
    cloudletFadeIn = 0;
    cloudletFadeOut = 0.3;
    cloudletAccY = -1;
    cloudletMinYSpeed = -10;
    cloudletMaxYSpeed = 10;

    cloudletShape = cloudletBlood;
    cloudletColor[] = {0.35, 0.00, 0.05, 0};

    size = 0.15;
    sourceSize = 0.05;
  };
  class CraterWater : CraterDustSmall
  {
    access = ReadAndWrite;
    interval = 0.01;
    cloudletAlpha = 1;
    cloudletDuration = 0.3;
    cloudletAnimPeriod = 1;
    cloudletGrowUp = 0.3;
    cloudletFadeIn = 0;
    cloudletFadeOut = 0.2;
    cloudletAccY = -10;
    cloudletMinYSpeed = -100;
    cloudletMaxYSpeed = 100;

    cloudletShape = cloudletWater;
    cloudletColor[] = {0.54, 0.69, 0.645, 0};

    size = 0.02;
    sourceSize = 0.02;
  };
  class CraterDustBig : CraterDustSmall
  {
    access = ReadAndWrite;
    cloudletDuration = 2;
    cloudletGrowUp = 0.2;
    cloudletFadeIn = 0.2;
    cloudletFadeOut = 1.5;
    cloudletAccY = -8;
    cloudletMinYSpeed = -20;
    cloudletMaxYSpeed = 10;
  };
  class CloudletsMissile
  {
    access = ReadAndWrite;
    interval = 0.005;
    cloudletSize = 1.7;
    cloudletAlpha = 0.5;
    cloudletDuration = 0.45;
    cloudletAnimPeriod = 1;
    cloudletGrowUp = 0.05;
    cloudletFadeIn = 0.0;
    cloudletFadeOut = 0.5;
    cloudletAccY = 0;
    cloudletMinYSpeed = -10;
    cloudletMaxYSpeed = 10;
    cloudletColor[] = {1, 1, 1, 0};
    initT = 0;
    deltaT = 0;
    class Table
    {
      class T0 {maxT = 0; color[] = {1, 1, 1, 0};}
    }
    cloudletShape = cloudletMissile;
  };
  class CloudletsMissileManual: CloudletsMissile
  {
    cloudletSize = 0.3;
    cloudletAlpha = 0.4;
  };
  class CloudletsDebugTrace: CloudletsMissile
  {
    cloudletSize = 0.02;
    cloudletAlpha = 0.5;
    cloudletDuration = 1.0;
    cloudletColor[] = {0.5, 0.5, 0, 0};
  };
  class CloudletsScud: CloudletsMissile
  {
    cloudletSize = 5;
    cloudletAlpha = 0.5;
    cloudletColor[] = {1, 1, 1, 0};

    cloudletFadeIn = 0;
    cloudletDuration = 1.5;
    cloudletFadeOut = 0.5;

    cloudletGrowUp = 1;

    cloudletAccY = 0;
    cloudletMinYSpeed = -1000;
    cloudletMaxYSpeed = 1000;

    interval = 0.02;
    size = 8.0;
    sourceSize = 1.0;
  };
  class DefaultSmoke
  {
    interval = 0.3; // sometimes overridden by program
    cloudletDuration = 1;
    cloudletAnimPeriod = 3;
    cloudletSize = 1; // sometimes overridden by program
    cloudletAlpha = 1;
    cloudletGrowUp = 0.3;
    cloudletFadeIn = 0.5;
    cloudletFadeOut = 2.0;
    cloudletAccY = 0;
    cloudletMinYSpeed = -10;
    cloudletMaxYSpeed = 10;
    cloudletColor[] = {1, 1, 1, 1};

    initT = 0;
    deltaT = 0;
    class Table {};

    cloudletShape = cloudletSmoke;

    density = 1; // sometimes overridden by program
    size = 1; // sometimes overridden by program
    timeToLive = 30;
    in = 1;
    out = 1;
    initYSpeed = 3.5; // sometimes overridden by program
  };
  class DestructedObjectDust
  {
    interval = 0.15;
    cloudletDuration = 0.5;
    cloudletAnimPeriod = 1;
    cloudletSize = 0.5; // sometimes overridden by program
    cloudletAlpha = 1;
    cloudletGrowUp = 0.4;
    cloudletFadeIn = 0.2;
    cloudletFadeOut = 1.5;
    cloudletAccY = -0.5;
    cloudletMinYSpeed = -1;
    cloudletMaxYSpeed = 1;
    cloudletColor[] = {0.25,0.23,0.16, 1};

    initT = 0;
    deltaT = 0;
    class Table {};

    cloudletShape = cloudletSmoke;

    density = 2;
    size = 0.5; // sometimes overridden by program
    timeToLive = 0.4; // sometimes overridden by program
    in = 0.8; // sometimes overridden by program
    out = 0.8; // sometimes overridden by program
    initYSpeed = 2.5; // sometimes overridden by program
  };
  class DestructedEngineSmoke
  {
    interval = 0.3; // overridden by program
    cloudletDuration = 0.5;
    cloudletAnimPeriod = 3;
    cloudletSize = 1; // overridden by program
    cloudletAlpha = 1;
    cloudletGrowUp = 0.5;
    cloudletFadeIn = 0.5;
    cloudletFadeOut = 3.0;
    cloudletAccY = -0.8;
    cloudletMinYSpeed = 0;
    cloudletMaxYSpeed = 5;
    cloudletColor[] = {0.15, 0.15, 0.10, 1}; // overridden by program

    initT = 0;
    deltaT = 0;
    class Table {};

    cloudletShape = cloudletSmoke;

    density = 1; // overridden by program
    size = 1; // overridden by program
    timeToLive = 6; // overridden by program
    in = 1; // overridden by program
    out = 6; // overridden by program
    initYSpeed = 3.5; // overridden by program
  };
};

class CfgOpticsEffect {};

class MissileManualEffects
{};
class MissileEffects
{};
class ImpactEffectsBig
{};
class ImpactEffectsSmall
{};
class ImpactEffectsBlood
{};
class ImpactEffectsWater
{};
class ExplosionEffects
{};
class ExhaustsEffect
{};
class ExhaustsEffectBig
{};
class LDustEffects
{};
class RDustEffects
{};
class LWaterEffects
{};
class RWaterEffects
{};
class LEngEffects
{};
class REngEffects
{};
class FDustEffects
{};
class HeliDust
{};
class HeliWater
{};
class DamageSmokeHeli
{};
class DamageSmokePlane
{};
class ScudEffect
{};


/// effect controlled destruction
class CfgDestructPos
{
  class DelayedDestruction
  {
    // we can use lifetime here if desired
    timeBeforeHiding = "0.2";
    hideDuration = "0.65+lifeTime*0.05";
  };
};

/// effect controlled indirect damage around destructed object
class CfgDamageAround
{
  class DamageAroundHouse
  {
    radiusRatio = "1.0";
    //indirectHit = "30*intensity";
    indirectHit = "11";
  };
};

// gun fire (used in tanks, ships etc.)
class WeaponFireGun
{
  access = ReadAndWrite;
  // single cloudlet parameters
  cloudletDuration = 0.2;   // time to live (not including fades)
  cloudletAnimPeriod = 1.0; // animation speed
  cloudletSize = 1.0;
  cloudletAlpha = 1.0;
  cloudletGrowUp = 0.2;     // grow-up time
  cloudletFadeIn = 0.01;    // fade-in time
  cloudletFadeOut = 0.5;    // fade-out time
  cloudletAccY = 0;         // vertical acceleration
  cloudletMinYSpeed = -100; // vertical speed range
  cloudletMaxYSpeed = 100;
  cloudletShape = cloudletFire;
  cloudletColor[] = {1, 1, 1, 0};
  // source parameters
  interval = 0.01;
  size = 3;
  sourceSize = 0.5;
  timeToLive = 0;

  initT = 4500;
  deltaT = -3000;
  class Table
  {
    class T0 {maxT = 0; color[] = {0.82, 0.95, 0.93, 0};}
    class T1 {maxT = 200; color[] = {0.75, 0.77, 0.9, 0};}
    class T2 {maxT = 400; color[] = {0.56, 0.62, 0.67, 0};}
    class T3 {maxT = 600; color[] = {0.39, 0.46, 0.47, 0};}
    class T4 {maxT = 800; color[] = {0.24, 0.31, 0.31, 0};}
    class T5 {maxT = 1000; color[] = {0.23, 0.31, 0.29, 0};}
    class T6 {maxT = 1500; color[] = {0.21, 0.29, 0.27, 0};}
    class T7 {maxT = 2000; color[] = {0.19, 0.23, 0.21, 0};}
    class T8 {maxT = 2300; color[] = {0.22, 0.19, 0.1, 0};}
    class T9 {maxT = 2500; color[] = {0.35, 0.2, 0.02, 0};}
    class T10 {maxT = 2600; color[] = {0.62, 0.29, 0.03, 0};}
    class T11 {maxT = 2650; color[] = {0.59, 0.35, 0.05, 0};}
    class T12 {maxT = 2700; color[] = {0.75, 0.37, 0.03, 0};}
    class T13 {maxT = 2750; color[] = {0.88, 0.34, 0.03, 0};}
    class T14 {maxT = 2800; color[] = {0.91, 0.5, 0.17, 0};}
    class T15 {maxT = 2850; color[] = {1, 0.6, 0.2, 0};}
    class T16 {maxT = 2900; color[] = {1, 0.71, 0.3, 0};}
    class T17 {maxT = 2950; color[] = {0.98, 0.83, 0.41, 0};}
    class T18 {maxT = 3000; color[] = {0.98, 0.91, 0.54, 0};}
    class T19 {maxT = 3100; color[] = {0.98, 0.99, 0.6, 0};}
    class T20 {maxT = 3300; color[] = {0.96, 0.99, 0.72, 0};}
    class T21 {maxT = 3600; color[] = {1, 0.98, 0.91, 0};}
    class T22 {maxT = 4200; color[] = {1, 1, 1, 0};}
  }
};

// machine gun fire (used in tanks, ships etc.)
class WeaponFireMGun : WeaponFireGun
{
  cloudletDuration = 0;     // time to live (not including fades)
  cloudletGrowUp = 0.06;    // grow-up time
  cloudletFadeIn = 0;       // fade-in time
  cloudletFadeOut = 0.12;   // fade-out time

  interval = 0.005;
  size = 0.12;
  sourceSize = 0.01;

  initT = 3200;
  deltaT = -4000;
};

// gun clouds (used in tanks, ships etc.)
class WeaponCloudsGun
{
  access = ReadAndWrite;
  // single cloudlet parameters
  cloudletDuration = 0.3;   // time to live (not including fades)
  cloudletAnimPeriod = 1.0; // animation speed
  cloudletSize = 1.0;
  cloudletAlpha = 1.0;
  cloudletGrowUp = 1;       // grow-up time
  cloudletFadeIn = 0.01;    // fade-in time
  cloudletFadeOut = 1;      // fade-out time
  cloudletAccY = 0.4;       // vertical acceleration
  cloudletMinYSpeed = 0.2;  // vertical speed range
  cloudletMaxYSpeed = 0.8;
  cloudletShape = cloudletClouds;
  cloudletColor[] = {1, 1, 1, 0};
  // source parameters
  interval = 0.05;
  size = 3;
  sourceSize = 0.5;
  timeToLive = 0;

  initT = 0;
  deltaT = 0;
  class Table
  {
    class T0 {maxT = 0; color[] = {1, 1, 1, 0};}
  }
};

// gun clouds (used in tanks, ships etc.)
class WeaponCloudsMGun : WeaponCloudsGun
{
  access = ReadAndWrite;

  cloudletGrowUp = 0.05;      // grow-up time
  cloudletFadeIn = 0;   // fade-in time
  cloudletFadeOut = 0.1;    // fade-out time
  cloudletDuration = 0.05;    // time to live (not including fades)

  cloudletAlpha = 0.3;

  cloudletAccY = 0;         // vertical acceleration
  cloudletMinYSpeed = -100; // vertical speed range
  cloudletMaxYSpeed = 100;

  interval = 0.02;
  size = 0.3;
  sourceSize = 0.02;
};

//Used for destruction effects.
class CfgLights
{
  class ObjectDestructionLight
  {
    color[] = {1.5, 0.75, 0};
    ambient[] = {0.45, 0.22, 0};
    brightness[] = {"fireIntensity", 0, 0.01};
  };
  class ExplosionLight
  {
    color[] = {1, 1, 1};
    ambient[] = {0.45, 0.22, 0};
    brightness[] = {"fireIntensity", 0, 0.01};
  };
};

/*!
\patch 1.14 Date 8/9/2001 by Ondra
- Fixed: Texture mapping on flags (about 1/8 of texture was skipped).
*/
class CfgCloth
{
  class Flag
  {
    //stepSize = 0.002; // high end - perfect stable
    stepSize = 0.005; // stable
    //stepSize = 0.01; // nearly stable
    //stepSize = 0.02; // slightly unstable
    colPoints = 9; // number of knots in x-direction
    rowPoints = 9; // number of knots in y-direction

    stretchCoef = 1000;  // stretch force coeficient
    fricCoef = 1; // friction coeficient
    windCoef = 10; // wind coeficient
    gravCoef = 1; // gravity coeficient
  };
};


// vehicles battle types
#define VSoft 0
#define VArmor 1
#define VAir 2

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

enum {StabilizedInAxesNone,StabilizedInAxisX,StabilizedInAxisY,StabilizedInAxesBoth}

#define CanSeeRadar 1
#define CanSeeEye 2
#define CanSeeOptics 4
#define CanSeeEar 8
#define CanSeeCompass 16
#define CanSeeRadarC CanSeeRadar+CanSeeCompass
#define CanSeePeripheral 32
#define CanSeeAll 31

/// defines how persons can switch positions in the vehicle
enum VehicleCompartments
{
  Compartment1 = 1,
  Compartment2 = 2,
  Compartment3 = 4,
  Compartment4 = 8
  // up to Compartment32 = 0x80000000
};

class CfgVehicleClasses
{
  access = ReadAndCreate;
  class Men
  {
    displayName = $STR_DN_MEN;
  };
  class Animals
  {
    displayName = $STR_DN_ANIMALS;
  };
  class Car
  {
    displayName = $STR_DN_CARS;
  };
  class Armored
  {
    displayName = $STR_DN_ARMORED;
  };
  class Air
  {
    displayName = $STR_DN_AIR;
  };
  class Support
  {
    displayName = $STR_DN_SUPPORT;
  };
  class Camera
  {
    displayName = $STR_DN_CAMERAS;
  };
  class Objects
  {
    displayName = $STR_DN_OBJECTS;
  };
  class Ammo
  {
    displayName = $STR_DN_AMMO;
  };
  class Sounds
  {
    displayName = $STR_DN_SOUNDS;
  };
  class Mines
  {
    displayName = $STR_DN_MINES;
  };
  class Ship
  {
    displayName = $STR_DN_SHIP;
  };
};

class CfgFactionClasses
{
        access = ReadAndCreate;
        class Default{displayName = $STR_disp_other;priority =10000;side=-1;};
};
/// HUD used for typical planes and helicopters
class AirplaneHUD
{
  /// HUD location memory points
  topLeft = "HUD LH";
  topRight = "HUD PH";
  bottomLeft = "HUD LD";
  /// adjust position inside of memory points LOD
  borderLeft = 0;
  borderRight = 0;
  borderTop = 0;
  borderBottom = 0;

  color[]={0,1,0,0.1}; // master color
  //font<=fontHelicopterHUD;
  //font<="FontConfig/HUD";

#define Pos0Center 0.27
  /// common definition for all vectors projected into HUD space
  class Pos10Vector
  {
    type=vector;
    // position of 0-degree dive line
    pos0[]={0.5,Pos0Center};
    // position of 10 degree bank and 10-degree dive (used to adjust scale)
    //pos10[]={2.0,Pos0Center+1.3};
    pos10[]={0.5+0.9,Pos0Center+0.7};
  };

  class Bones
  {
    class AGLMove1
    {
      type=linear;
      source=altitudeAGL; // above ground level
      min=0;
      max=100;
      minPos[]={0.05,0.1};
      maxPos[]={0.05,0.8};
    };
    class AGLMove2
    {
      type=fixed;
      pos[]={0.05,0.8};
    };
    class ASLMove1
    {
      type=linear;
      source=altitudeASL; // above sea level
      min=0;
      max=500;
      minPos[]={0.1,0.1};
      maxPos[]={0.1,0.8};
    };
    class ASLMove2
    {
      type=fixed;
      pos[]={0.1,0.8};
    };
    class VertSpeed
    {
      type=linear;
      source=vSpeed;
      min=-25;
      max=25;
      minPos[]={0,-0.4};
      maxPos[]={0,0.4};
    };
    class SpdMove2
    {
      source=speed;
      min=33;
      max=200;
      type=linear;
      minPos[]={0.94,0.1};
      maxPos[]={0.94,0.87};
    };
    class ILS
    {
      type=ils;
      // position when centered
      pos0[]={0.5,0.4};
      // position when 3-degree off in both directions
      pos3[]={0.7,0.6};
    };
    class WeaponAim: Pos10Vector
    {
      source=weapon;
    };
    class Target: Pos10Vector
    {
      source=target;
    };
    class TargetDistanceMissile
    {
      type=rotational;
      source=targetDist;
      center[]={0,0};
      min=100;
      max=3000;
      minAngle=-120;
      maxAngle=+120;
    };
    class TargetDistanceMGun
    {
      type=rotational;
      source=targetDist;
      center[]={0,0};
      min=100;
      max=1000;
      minAngle=-180;
      maxAngle=+90;
    };
    class Level0: Pos10Vector
    {
      type=horizon;
      // horizon angle to which this line corresponds to
      angle=0;
    };
    class LevelP5: Level0 {angle=+5;};
    class LevelM5: Level0 {angle=-5;};
    class LevelP10: Level0 {angle=+10;};
    class LevelM10: Level0 {angle=-10;};
    class LevelP15: Level0 {angle=+15;};
    class LevelM15: Level0 {angle=-15;};
    class Velocity: Pos10Vector
    {
      type=vector;
      source=velocity;
    };
    class PlaneW
    {
      type=fixed;
      pos[]={0.5,Pos0Center};
    };
  };
  class Draw
  {
    alpha=0.8;
    color[]={0.1,0.5,0.05};
    clipTL[]={0.0,0.05};
    clipBR[]={1.0,0.90};
    condition="on";
    class Altitude
    {
      type=line;
      points[]={
        {AGLMove1,1},{AGLMove2,1},{},
        {ASLMove2,1},{ASLMove1,1},
        // climb rate shown here as well
        {ASLMove1,{0.02,0},1},{ASLMove1,{0.02,0},1,VertSpeed,1}
      };
    };
    class DimmedBase
    {
      alpha=0.3;
      class AltitudeBase
      {
        type=line;
        points[]={{AGLMove2,1},{ASLMove2,1}};
      };
    };
    class Speed
    {
      type=line;
      points[]={
        {{0.95,0.87},1},{{0.95,0.1},1},{},
        {SpdMove2,{-0.05,0},1},{SpdMove2,1}
      };
    };
    class SpeedNumber
    {
      type=text;
      align=left;
      scale=1; // text size
      source=speed;
      sourceScale=3.6; // convert from m/s to km/h
      pos[]={SpdMove2,{-0.05,-0.03},1};
      right[]={SpdMove2,{0.01,-0.03},1};
      down[]={SpdMove2,{-0.05,0.03},1};
    };
    class PlaneW
    {
      clipTL[]={0.0,0.1};
      clipBR[]={1.0,0.90};
      class LineHL
      {
        type=line;
        points[]={
          /*{{0.43,Pos0Center},1},{{0.48,Pos0Center},1},
          {{0.5,Pos0Center-0.02},1},{{0.52,Pos0Center},1},
          {{0.5,Pos0Center+0.02},1},{{0.48,Pos0Center},1},{},
          {{0.52,Pos0Center},1},{{0.57,Pos0Center},1}*/
          {PlaneW, {-0.07, 0}, 1}, {PlaneW, {-0.02, 0}, 1},
          {PlaneW, {0, -0.02}, 1}, {PlaneW, {+0.02, 0}, 1},
          {PlaneW, {0, +0.02}, 1}, {PlaneW, {-0.02, 0}, 1}, {},
          {PlaneW, {+0.02, 0}, 1}, {PlaneW, {+0.07, 0}, 1}
        };
      };
      class Velocity
      {
        type=line;
        points[]={
          {Velocity,{0,-0.02},1},{Velocity,{0.02,0},1},
          {Velocity,{0,+0.02},1},{Velocity,{-0.02,0},1},
          {Velocity,{0,-0.02},1}
        };
      };
    };
    class MGun
    {
#define CIRCLE(bone,scale) \
      {bone,{0,-scale},1},{bone,{+0.7*scale,-0.7*scale},1}, \
      {bone,{+scale,0},1},{bone,{+0.7*scale,+0.7*scale},1}, \
      {bone,{0,+scale},1},{bone,{-0.7*scale,+0.7*scale},1}, \
      {bone,{-scale,0},1},{bone,{-0.7*scale,-0.7*scale},1}, \
      {bone,{0,-scale},1}
#define CIRCLE_MARK(bone,scaleIn,scaleOut,angle) \
      {bone,{scaleIn*sin(angle),-scaleIn*cos(angle)},1}, \
      {bone,{scaleOut*sin(angle),-scaleOut*cos(angle)},1}

      condition="mgun";
      class Circle
      {
        type=line;
        points[]={
          CIRCLE(WeaponAim,0.07),{},
          CIRCLE(WeaponAim,0.01),{},
          CIRCLE_MARK(WeaponAim,0.03,0.07,-180),{},
          CIRCLE_MARK(WeaponAim,0.03,0.07,+90),{},
          {WeaponAim,1,TargetDistanceMGun,{0,0.04},1},
          {WeaponAim,1,TargetDistanceMGun,{0,0.07},1}
        };
      };
    };
    class Missile
    {
      condition="missile";
      class Circle
      {
        type=line;
        points[]={
          CIRCLE(WeaponAim,0.1),{},
          CIRCLE_MARK(WeaponAim,0.1*0.8,0.1*1.2,-120),{},
          CIRCLE_MARK(WeaponAim,0.1*0.8,0.1*1.2,+120),{},
          {WeaponAim,1,TargetDistanceMissile,{0,0.1*0.8},1},
          {WeaponAim,1,TargetDistanceMissile,{0,0.1*1.2},1}
        };
      };
      class Target
      {
        type=line;
        points[]={
          {Target,{-0.05,-0.05},1},
          {Target,{+0.05,-0.05},1},
          {Target,{+0.05,+0.05},1},
          {Target,{-0.05,+0.05},1},
          {Target,{-0.05,-0.05},1}
        };
      };
    };
    class Horizont
    {
      clipTL[]={0.2,0.1};
      clipBR[]={0.8,0.9};
      class Dimmed
      {
        alpha=0.6;
        class Level0
        {
          type=line;
          points[]={
            {Level0,{-0.40,0},1},{Level0,{-0.13,0},1},{},
            {Level0,{+0.13,0},1},{Level0,{+0.40,0},1}
          };
        };
      };
      class LevelP5
      {
        type=line;
        points[]={
          {LevelP5,{-0.15,+0.03},1},
          {LevelP5,{-0.15,0},1},{LevelP5,{+0.15,0},1},
          {LevelP5,{+0.15,+0.03},1}
        };
      };
      class LevelM5
      {
        type=line;
        points[]={
          {LevelM5,{-0.15,-0.03},1},
          {LevelM5,{-0.15,0},1},{LevelM5,{+0.15,0},1},
          {LevelM5,{+0.15,-0.03},1}
        };
      };
      class LevelP10
      {
        type=line;
        points[]={
          {LevelP10,{-0.20,+0.05},1},
          {LevelP10,{-0.20,0},1},{LevelP10,{+0.20,0},1},
          {LevelP10,{+0.20,+0.05},1}
        };
      };
      class LevelM10
      {
        type=line;
        points[]={
          {LevelM10,{-0.20,-0.05},1},
          {LevelM10,{-0.20,0},1},{LevelM10,{+0.20,0},1},
          {LevelM10,{+0.20,-0.05},1}
        };
      };
      class LevelP15
      {
        type=line;
        points[]={
          {LevelP15,{-0.20,+0.07},1},
          {LevelP15,{-0.20,0},1},{LevelP15,{+0.20,0},1},
          {LevelP15,{+0.20,+0.07},1}
        };
      };
      class LevelM15
      {
        type=line;
        points[]={
          {LevelM15,{-0.20,-0.07},1},
          {LevelM15,{-0.20,0},1},{LevelM15,{+0.20,0},1},
          {LevelM15,{+0.20,-0.07},1}
        };
      };
    };
    class ILS
    {
      condition="ils";
      class Glideslope
      {
        clipTL[]={0.29,0.29};
        clipBR[]={0.71,0.71};
        class ILS
        {
          type=line;
          points[]={
            {ILS,{-10,0},1},{ILS,{+10,0},1},{},
            {ILS,{0,-10},1},{ILS,{0,+10},1}
          };
        };
      };
      class AOABracket
      {
        type=line;
        points[]={
          {{0.42,0.78},1},{{0.4,0.78},1},
          {{0.4,0.88},1},{{0.42,0.88},1}
        };
      };
    };
  };
}; // HUD

enum {CPDriver, CPGunner, CPCommander, CPCargo};

// vehicle types
class CfgVehicles
{
  // define enum value for XML docs
  //-->Vehicle
  access = ReadAndCreate;
  //default definitions valid for all units
  // NOTE: DO NOT enter any classes other than vehicle types

  /*!
  \patch 1.28 Date 10/22/2001 by Ondra
  - Fixed: Tank drivers were not able to see enemy.
  */
  class All
  {
    //-- any vehicle
    access = ReadAndWrite;

    scope=private;
    reversed=true; // all vehicles are reversed - except static
    autocenter=true; // normal vehicle are autocentered
    animated=true; // model is animated and cannot be assumed static
    shadow=true; // by default shadows are enabled

    mapSize = 10;
    featureSize = 0;

    vehicleClass="";
    targetCategory="";

    icon = iconObject;

    side=TSideUnknown;
    faction = "Default";
    displayName=$STR_DN_UNKNOWN;
    nameSound="unknown";
    speechSingular[] = {};
    speechPlural[] = {};
    textSingular = "";
    textPlural = "";

    weaponSlots = 0;
    clothingType = "";

    storeVariables[] = {};
    synchronizedVariables[] = {};

    // targets recognition
    camouflage=2; // how difficult to spot - bigger - better spotable
    audible=1; // audible - bigger - better heard
    accuracy=0; // accuracy needed to recognize type of this target

    // night spotability coefficients
    spotableDarkNightLightsOff=0.003; // 0.02 was too much, soldier was seen in a dark night
    spotableNightLightsOff=0.035;
    spotableNightLightsOn=4;

    // night target recognition (currently not used)
    visibleNightLightsOff=0.1;
    visibleNightLightsOn=0.2;

    // how is the volume of the vehicle engine/movement changed when sitting inside that vehicle
    // by default we play our own voices with half volume
    insideSoundCoef = 0.5;
    // enable low pass filter to surrounding samples when camera is located inside vehicle, false = disabled
    outsideSoundFilter = false;

    /**a vehicle can occlude / obstruct sounds when sound is outside of the vehicle
    and listener inside, or the other way around
    obstruction: how much is the direct path free
    there is some indirect way how can the sound reach the vehicle (open vehicle window),
    but the direct path is not available (obstruction leaves echoed sound unfiltered)
    occlusion: how much is both the direct and indirect path free
    describes how much the vehicle is sealed (echoed sound is filtered as well)
    1 means not occluded, 0 means fully occluded (db-XX can also be used)
    See also EAX documentation.
    */

    occludeSoundsWhenIn = db-10;
    obstructSoundsWhenIn = db-5;
    /**
    How much are low frequencies attenuated by obstruction and occlusion.
    0 = no attenuation (LF goes through), 1 = full attenuation (LF attenuated the same way as HF)
    Note: not implemented yet, 0 and 0.25 is currently used.
    */
    obstructSoundLFRatio = 0;
    occludeSoundLFRatio = 0.25;

    unloadInCombat = false;

    gunnerHasFlares = true; // required parameter for tanks
    enableManualFire = true; // required parameter for transports - Manual Fire action is available

    // vehicle sensitivity, most vehicles see and hear less than man
    sensitivity=2.5; // sensor sensitivity
    sensitivityEar=0.0075;

    // default properties
    model=bmp; // typical size vehicle
    picture=; // use picture from model
    portrait=; // Picture in Command Bar

    ghostPreview=; // Special object for COIN constructing module

    simulation=;
    destrType=DestructDefault;
    armor=100;
    armorLights=0.4;
    armorStructural=1.0; // structural dammage
    crewVulnerable = true;
    damageResistance = 0.004;

    // destruction of buildings
    replaceDamaged = ""; // what object will be building replaced by
    replaceDamagedLimit = 0.9; // limit for damage of hitpoint from the list below when the building is replaced
    replaceDamagedHitpoints[] = {}; // list of hitpoints to check for limit

    cost=500000;
    fuelCapacity=0;
    //ammount of fuel needed for 1 second, so far used only for RTD heli
    fuelConsumptionRate = 0.01f;

    extCameraPosition[]={0,2,-20};
    groupCameraPosition[]={0,5,-30};

    steerAheadSimul=0.3; // steer point on path properties
    steerAheadPlan=0.4;

    minFireTime=20; // minimal time spent firing on single target

    predictTurnSimul=1.2; // brake before turns
    predictTurnPlan=1.0;

    precision=5; // vehicle movement precision
    brakeDistance=5; // vehicle movement precision
    formationX=10; // formation spacing
    formationZ=20;

    formationTime=5;

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

    preferRoads=false; // never used value, engine don't use it
    unitInfoType=RscUnitInfoTank;
    unitInfoTypeLite = 0;  //unitInfoTypelite = RscUnitInfoTankLite; if unitInfoTypeLite = 0, unitInfoType will be used
    hideUnitInfo=false;
    nightVision=false;

    driverCanEject = true;
    cargoCanEject = true;

    commanderCanSee = CanSeeAll;
    gunnerCanSee = CanSeeOptics+CanSeeEar+CanSeeCompass;
    driverCanSee = CanSeeEye+CanSeeEar+CanSeeCompass;
    //0 - none, 1 - tactical display, 2 - vehicle radar, 4 - air radar
    radarType = 0;

    maxSpeed=80; // max speed on level road, km/h
    /// how fast to move when speed mode is SpeedLimited (relative to _maxSpeed)
    limitedSpeedCoef = 0.22;
    secondaryExplosion=-1;
    // value<0: calculate from fuel/ammo cargo + tanks and multiply with abs(value)
    // value>0: use value directly as hit in FuelExplosion

    hasDriver=true;
    driverForceOptics=false;

    // hide weapons when unit is sitting on given position
    hideWeaponsDriver = true;
    hideWeaponsCargo = false;

    getInRadius = 2.5;

    selectionClan = "";
    selectionDashboard = "";
    selectionShowDamage = "";
    selectionBackLights = "";

    memoryPointSupply = "doplnovani";
    /** when supplyRadius is positive, it is directly the radius of the supply circle
    when it is negative, the radius is calculated as geometry lod radius * -supplyRadius
    */
    supplyRadius = -1;

    // Notice: When showGPS in mission is true, the GPS is enabled in any case
    enableGPS = false; // default vehicle has not GPS miniMap enabled
    enableWatch = false; // watch device is not present by default
    enableRadio = false; // radio station is not present by default

    //which weapons lock can vehicle detect
    lockDetectionSystem = 0;
    incommingMisslieDetectionSystem = 0;

    //if precise action interaction test. should be uset for getInXX actions
    usePreciseGetInAction = false;
    allowTabLock = true;
    showAllTargets = showAllNo;

    wheelCircumference = 1;

    class EventHandlers{};

    class MarkerLights
    {
    };

    // light sources - not targets
    class NVGMarkers {}

    // define Strobe/IRStrobe target pars
    class NVGMarker
    {
      diffuse[] = {1, 1, 1, 1};
      ambient[] = {1, 1, 1, 1};
      brightness = 1;
      blinking = false;
      onlyInNvg = false;
    };

    // new style of turrets definition - structure description
    class Turrets // no turrets by default
    {
    };

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
    class ViewCargo
    {
      initAngleX=5; minAngleX=-85; maxAngleX=+85; //Rg 30;
      initAngleY=0; minAngleY=-150; maxAngleY=+150; //Rg 125;
      initFov=0.7; minFov=0.42; maxFov=0.85;
      minMoveX=0; maxMoveX=0; minMoveY=0; maxMoveY=0; minMoveZ=0; maxMoveZ=0;
    };
    class ViewOptics
    {
      initAngleX=0; minAngleX=-30; maxAngleX=+30;
      initAngleY=0; minAngleY=-100; maxAngleY=+100;
      initFov=0.7; minFov=0.42; maxFov=0.85;
      minMoveX=0; maxMoveX=0; minMoveY=0; maxMoveY=0; minMoveZ=0; maxMoveZ=0;
    };
    // specific data for person relative to given vehicle position (can differ for standalone Person, Car, Tank, Helicopter, Ship, Parachute, ...)
    class PilotSpec
    {
      showHeadPhones = false;
    };
    class CargoSpec
    {
      class Cargo1
      {
        showHeadPhones = false;
      };
    };

    // other properties
    transportSoldier = 0;
    transportAmmo = 0;

    // no cargo space by default
    itemsCargoSize[] = {0, 0}; 

    transportFuel = 0;
    transportRepair = 0;
    transportVehiclesCount = 0;
    transportVehiclesMass = 0;
    class TransportWeapons {};
    class TransportMagazines {};
    attendant = false;
    engineer = false;

    soundEngine[] = {"", 1, 1};
    soundEnviron[] = {"", 1, 1};
    class SoundEnvironExt
    {
      // use soundEnviron as default
    };

    class SoundEquipment {};
    class SoundGear {};
    class SoundBreath {};

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

    // default vehicle has no weapons
    weapons[]={};
    magazines[]={};

    type=VArmor;
    //threat[] VSoft, VArmor, VAir
    threat[]={0.7, 0.5, 0.3};

    typicalCargo[]={};

    class Reflectors {};
    aggregateReflectors[] = {};

    // get in / out actions for the driver / pilot
    getInAction = "";
    getOutAction = "";

    driverAction = "";
    driverInAction = "";

    // get in / out actions for cargo
    cargoGetInAction[] = {};
    cargoGetOutAction[] = {};

    cargoAction[] = {};
    cargoIsCoDriver[] = {false};

    driverCompartments = Compartment1;
    cargoCompartments[] = {Compartment1};

    driverOpticsModel = "";
    driverOpticsEffect[] = {};

    driverOpticsColor[] = {1, 1, 1, 1};

    hideProxyInCombat = false;
    forceHideDriver = false;

    //@{ shadows should be casted in some special internal (cockpit) views
    // diffuse light may be attenuated when rendering interior
    // ambient light attenuation not implemented yet
    /// used for VIEW_PILOT
    viewDriverShadow = 1;
    viewDriverShadowDiff = 1.0;
    viewDriverShadowAmb = 1.0;

    /// used for VIEW_CARGO
    viewCargoShadow = 1;
    viewCargoShadowDiff = 1.0;
    viewCargoShadowAmb = 1.0;
    //@}

    //@{ some crew position should be ejected when killed
    ejectDeadDriver = false;
    ejectDeadCargo = false;
    //@}

    crew = "Civilian";

    // selections, where texture can be redefined
    hiddenSelections[] = {};
    // initial texture in hidden selections
    hiddenSelectionsTextures[] = {};
	hiddenSelectionsMaterials[] = {};

    /// definition of explosion effects
    class FxExplo
    {
      access = ReadAndCreate;
    };

    class AnimationSources {};
    selectionFireAnim = "";
    class GunFire : WeaponFireGun {};
    class GunClouds : WeaponCloudsGun {};
    class MGunClouds : WeaponCloudsMGun {};

    /// material animation based on total (structural) vehicle damage
    class Damage
    {
      tex[]={};
      mat[]={};
    };
    /// selection used for total damage (parts of the model connected to no hitzone)
    selectionDamage="zbytek";

   // if is driver and in cocpit force view down by headAimDown
    HeadAimDown = 0;

    class DestructionEffects
    {
      class Light1
      {
        simulation = "light";
        type = "ObjectDestructionLight";
        position = "destructionEffect1";

        intensity = 1;
        interval = 1;
        lifeTime = 1;
      };
      class Smoke1
      {
        simulation = "particles";
        type = "ObjectDestructionSmoke";
        position = "destructionEffect1";

        intensity = 1;
        interval = 1;
        lifeTime = 1;
      };
      class Fire1
      {
        simulation = "particles";
        type = "ObjectDestructionFire";
        position = "destructionEffect1";

        intensity = 1;
        interval = 1;
        lifeTime = 1;
      };
      class Sparks1
      {
        simulation = "particles";
        type = "ObjectDestructionSparks";
        position = "destructionEffect1";

        intensity = 1;
        interval = 1;
        lifeTime = 1;
      };
      class Sound
      {
        simulation = "sound";
        type = "";
        position = "destructionEffect1";

        intensity = 1;
        interval = 1;
        lifeTime = 1;
      };
    };
  };

  class Logic : All
  {
    //-- game logic invisible unit
    displayName = $STR_DN_LOGIC;
    nameSound = "unknown";
    scope = public;
    vehicleClass = "Objects";
    simulation = "invisible";
    side = TLogic;
    icon = "#(argb,8,8,3)color(1,1,1,1)";
    model = "\core\default\default.p3d";
    picture = "#(argb,8,8,3)color(1,1,1,1)";

    fsmFormation = "";
    fsmDanger = "";
  };

  class AllVehicles: All
  {
    icon = iconVehicle;
    nameSound="unknown";

    class SquadTitles {name="clan_sign"; /*font<=fontClanName;*/ color[]={0,0,0,0.75};}

    memoryPointDriverOptics[] = {"driverview", "pilot"};

    //if precise GetIn action is enabled, these are used for AI
    memoryPointsGetInDriver = "pos driver";
    memoryPointsGetInDriverDir = "pos driver dir";
    memoryPointsGetInCargo = "pos cargo";
    memoryPointsGetInCargoDir = "pos cargo dir";
    memoryPointsGetInCoDriver = "pos codriver";
    memoryPointsGetInCoDriverDir = "pos codriver dir";

    //if precise GetIn action is enabled, these are used for player (dor handle position)
    memoryPointsGetInDriverPrecise = "pos driver";
    // precise get in cargo allows only array (memoryPointsGetInCoDriver is not supported)
    memoryPointsGetInCargoPrecise[] = {"pos cargo"};

	  //memory points for car, tank or ship water effects leftWaterEffect and rightWaterEffect
    memoryPointsLeftWaterEffect = "waterEffectL";
    memoryPointsRightWaterEffect = "waterEffectR";

    selectionClan = "clan";
    selectionDashboard = "podsvit pristroju";
    selectionShowDamage = "poskozeni";
    selectionBackLights = "zadni svetlo";

    class NewTurret
    {
      body = "mainTurret";
      gun = "mainGun";
      animationSourceBody = "mainTurret";
      animationSourceGun = "mainGun";
      animationSourceHatch = "hatchGunner";

      proxyType = CPGunner;
      proxyIndex = 1;
      gunnerName = $STR_POSITION_GUNNER;

      primaryGunner = 1;
      primaryObserver = 0;

      weapons[]= {};
      magazines[]= {};


      soundServo[] = {"", db-50, 1.0};

      gunBeg = ""; //gunBeg = endpoint of the gun
      gunEnd = ""; //gunEnd = chamber of the gun

      minElev=-4; maxElev=+20; initElev=0;
      minTurn=-360; maxTurn=+360; initTurn=0;
      maxHorizontalRotSpeed = 1.2; maxVerticalRotSpeed = 1.2;

      stabilizedInAxes = StabilizedInAxesBoth;

      primary = true; //Commander can take over with "manual fire"
      hasGunner = true; //gunner position is accessible by Get In
      commanding = true; // how much gunner commands the vehicle

      // get in / out actions for the gunner
      gunnerGetInAction = "";
      gunnerGetOutAction = "";

      gunnerAction = "";
      gunnerInAction = "";
      class ViewGunner
      {
        initAngleX=5; minAngleX=-85; maxAngleX=+85; //Rg 30;
        initAngleY=0; minAngleY=-150; maxAngleY=+150; //Rg 100;
        initFov=0.7; minFov=0.42; maxFov=0.85;
        minMoveX=0; maxMoveX=0; minMoveY=0; maxMoveY=0; minMoveZ=0; maxMoveZ=0;
        continuous = 0; //no zoom by default. The value 8 means the FOV is 8x zoomed in 1 second (in ArmA the 8 is engine default for continuous CameraTypes)
      };
      class TurretSpec
      {
        showHeadPhones = false;
      };

      gunnerOpticsModel = "";
      gunnerOpticsColor[] = {0, 0, 0, 1};
      memoryPointGunnerOptics= "";
      gunnerForceOptics = true;
      gunnerOpticsShowCursor = 0;
      turrentInfoType = "";

      gunnerOutOpticsModel = "";
      gunnerOutOpticsColor[] = {0, 0, 0, 1};
      gunnerOpticsEffect[] = {};
      gunnerOutOpticsEffect[] = {};

      memoryPointGunnerOutOptics= "";
      gunnerOutForceOptics = 0;
      gunnerOutOpticsShowCursor = 0;
      gunnerFireAlsoInInternalCamera = 1;
      gunnerOutFireAlsoInInternalCamera = 1;


      gunnerUsesPilotView = 0;
      castGunnerShadow = 0;

      viewGunnerShadow = 1;
      viewGunnerShadowDiff = 1.0;
      viewGunnerShadowAmb = 1.0;

      ejectDeadGunner = 0;
      hideWeaponsGunner = true;
      /// gunner can hide when forceHideGunner set or hideProxyInCombat set and combat mode present
      canHideGunner = true;
      forceHideGunner = 0;
      outGunnerMayFire = 0;
      inGunnerMayFire = true;
      
      //by default units can eject from turret
      canEject = true;

      /*
      Requires hideProxyInCombat=1;
      Renders gunner also in external view even when turned in.

      Caution: Used to simulate the gunners which cannot be turned in, ued also for sound occlusion decision.
      */    viewGunnerInExternal = 0;

      lockWhenDriverOut = 0;

      // when speed of vehicle is higher that this value then gunner cannot control movement of his turret, -1 for disable
      lockWhenVehicleSpeed = -1;

      gunnerCompartments = Compartment1;

      LODTurnedIn = -1;
      LODTurnedOut = -1;

      /// switch vehicle engine on when this turret moved
      startEngine = true;

      memoryPointsGetInGunner= "";
      memoryPointsGetInGunnerDir= "";
      memoryPointsGetInGunnerPrecise= "";

      memoryPointGun = "";
      selectionFireAnim = "";

      missileBeg = "spice rakety";
      missileEnd = "konec rakety";

      armorLights=0.4;
      class Reflectors {};
      aggregateReflectors[] = {};

      class GunFire : WeaponFireGun {};
      class GunClouds : WeaponCloudsGun {};
      class MGunClouds : WeaponCloudsMGun {};

      class HitPoints
      {
        class HitTurret {armor=0.8;material=51;name=turret;visual="turret";passThrough=1;};
        class HitGun {armor=0.6;material=52;name=gun;visual="gun";passThrough=1;};
      };
      class Turrets {};
      class ViewOptics
      {
        initAngleX=0; minAngleX=-30; maxAngleX=+30;
        initAngleY=0; minAngleY=-100; maxAngleY=+100;
        initFov=0.3; minFov=0.07; maxFov=0.35;
        minMoveX=0; maxMoveX=0; minMoveY=0; maxMoveY=0; minMoveZ=0; maxMoveZ=0;
      };

      // set to TRUE disable using vision modes when player has NVG on
      forceNVG = false;
      
      // enables player to take control over vehicle
      isCopilot = false;
      gunnerLeftHandAnimName = "";
      gunnerRightHandAnimName = "";
      gunnerLeftLegAnimName = "";
      gunnerRightLegAnimName = "";

      gunnerDoor = "";
      preciseGetInOut = 0;
      turretFollowFreeLook = false;

      allowTabLock = true;
      showAllTargets = showAllNo;
    };
    class ViewPilot
    {
      initAngleX=5; minAngleX=-85; maxAngleX=+85; //Rg 30;
      initAngleY=0; minAngleY=-150; maxAngleY=+150; //Rg 100;
      initFov=0.7; minFov=0.42; maxFov=1.2;
      minMoveX=0; maxMoveX=0; minMoveY=0; maxMoveY=0; minMoveZ=0; maxMoveZ=0;
    };
    class ViewCargo
    {
      initAngleX=5; minAngleX=-85; maxAngleX=+85; //Rg 30;
      initAngleY=0; minAngleY=-150; maxAngleY=+150; //Rg 125;
      initFov=0.7; minFov=0.42; maxFov=0.85;
      minMoveX=0; maxMoveX=0; minMoveY=0; maxMoveY=0; minMoveZ=0; maxMoveZ=0;
    };
    class ViewOptics
    {
      initAngleX=0; minAngleX=-30; maxAngleX=+30;
      initAngleY=0; minAngleY=-100; maxAngleY=+100;
      initFov=0.7; minFov=0.42; maxFov=0.85;
      minMoveX=0; maxMoveX=0; minMoveY=0; maxMoveY=0; minMoveZ=0; maxMoveZ=0;
    };
    class PilotSpec
    {
      showHeadPhones = false;
    };
    class CargoSpec
    {
      class Cargo1
      {
        showHeadPhones = false;
      };
    };

    // by default there is no MFD/HUD
    class MFD
    {
    };

    class Sounds
    {
      class Engine
      {
        sound = "soundEngine";
        frequency = "rpm";
        volume = "rpm + speed";
      };
      class Movement
      {
        sound = "soundEnviron";
        frequency = "speed";
        volume = "speed";
      };
    };

    class SoundEvents {};

    tracksSpeed = 0; // car mostly don't have tracks, to  enable half-truck set bigger then 1
    selectionLeftOffset = "";
    selectionRightOffset = "";

    class RenderTargets {};

    cargoProxyIndexes[] = {};
    driverLeftHandAnimName = "";
    driverRightHandAnimName = "";
    driverLeftLegAnimName = "";
    driverRightLegAnimName = "";

    driverDoor = ;
    cargoDoors[] = {};

    preciseGetInOut = 0;
    cargoPreciseGetInOut[] = {0};

  };

  class Land: AllVehicles
  {
    //-- ground vehicle
    displayName=$STR_DN_VEHICLE;
    nameSound="unknown";
    accuracy=0.0005;
    armor=30;
    cost=100000;
    fuelCapacity=0;
    //weapons[]={Gun120,GunHeat120,MachineGun12_7}; // be carefull - rather expect good tank
    weapons[]={FakeWeapon}; // if we do not know what it is, it is probably soldier
    magazines[]={FakeWeapon};
    type=VArmor;
    //threat[] VSoft, VArmor, VAir
    threat[]={0.8, 0.8, 0.3};
  };
  class LandVehicle: Land
  {
    displayName=$STR_DN_VEHICLE;
    accuracy=0.02;

    cost=500000;

    selectionBrakeLights = "brzdove svetlo";

    //1st value is name of point in memory LOD, if this point doesn't exist use 2nd one //used only in TankOrCar
    memoryPointMissile[] = {"spice rakety", "usti hlavne"};
    memoryPointMissileDir[] = {"konec rakety", "konec hlavne"};

    class Exhausts
    {
      class Exhaust1
      {
        position = "vyfuk start";
        direction = "vyfuk konec";
        effect = "ExhaustsEffect";
      };
    };

    leftDustEffect = "LDustEffects";
    rightDustEffect = "RDustEffects";

    leftWaterEffect = "LWaterEffects";
    rightWaterEffect = "RWaterEffects";

    class Reflectors
    {
      class Left
      {
        color[] = {0.9, 0.8, 0.8, 1.0};
        ambient[] = {0.1, 0.1, 0.1, 1.0};
        position = "L svetlo";
        direction = "konec L svetla";
        hitpoint = "L svetlo";
        selection = "L svetlo";
        size = 0.5;
        brightness =0.5;
      };
      class Right
      {
        color[] = {0.9, 0.8, 0.8, 1.0};
        ambient[] = {0.1, 0.1, 0.1, 1.0};
        position = "P svetlo";
        direction = "konec P svetla";
        hitpoint = "P svetlo";
        selection = "P svetlo";
        size = 0.5;
        brightness = 0.5;
      };
    };
    aggregateReflectors[] = {{"Left", "Right"}};

    /// how fast tracks are moving relative to wheels
    tracksSpeed = 0; // car mostly don't have tracks, to  enable half-truck set bigger then 1
    selectionLeftOffset = "PasOffsetL";
    selectionRightOffset = "PasOffsetP";

  };
  class Car: LandVehicle
  {
    //--
    mapSize = 5;

    icon = iconCar;
    displayName=$STR_DN_CAR;
    nameSound="veh_car";
    accuracy=0.20;
    cost=40000;
    fuelCapacity=100;

    unloadInCombat = true;
    canFloat=false;

    armor=20; // total armor
    armorStructural=4.0; // very low structural dammage

    driverCanSee = CanSeeEar+CanSeeEye+CanSeePeripheral;
    gunnerCanSee = CanSeeEye+CanSeeEar+CanSeePeripheral;

    holdOffroadFormation = false;//evan if aware, cars stays on road

    class PlateInfos {name="spz"; /*font<=fontPlate;*/ color[]={0,0,0,0.75};}

    class HitPoints
    {
      class HitEngine {armor=1.2;material=60;name="engine";visual="engine";passThrough=1;}

      // note: glass is using other config entry (historical reasons)
      // use visual="" unless you want to animate material
      class HitRGlass {armor=0.5;material=-1;name="sklo predni P";convexComponent="sklo predni P";visual="";passThrough=1;}
      class HitLGlass {armor=0.5;material=-1;name="sklo predni L";convexComponent="sklo predni L";visual="";passThrough=1;}

      class HitBody {armor=0.4;material=50;name="karoserie";visual="karoserie";passThrough=1;}
      class HitFuel {armor=1.4;material=-1;name="palivo";visual="palivo";passThrough=1;}

      class HitLFWheel {armor=0.05;material=-1;name="Levy predni tlumic";visual="Levy predni";passThrough=1;}
      class HitRFWheel {armor=0.05;material=-1;name="Pravy predni tlumic";visual="Pravy predni";passThrough=1;}

      class HitLF2Wheel {armor=0.05;material=-1;name="Levy dalsi tlumic";visual="Levy dalsi";passThrough=1;}
      class HitRF2Wheel {armor=0.05;material=-1;name="Pravy dalsi tlumic";visual="Pravy dalsi";passThrough=1;}

      class HitLMWheel {armor=0.05;material=-1;name="Levy prostredni tlumic";visual="Levy prostredni";passThrough=1;}
      class HitRMWheel {armor=0.05;material=-1;name="Pravy prostredni tlumic";visual="Pravy prostredni";passThrough=1;}

      class HitLBWheel {armor=0.05;material=-1;name="Levy zadni tlumic";visual="Levy zadni";passThrough=1;}
      class HitRBWheel {armor=0.05;material=-1;name="Pravy zadni tlumic";visual="Pravy zadni";passThrough=1;}
    };

    selectionFireAnim = "zasleh";

    alphaTracks = 0.2;
    memoryPointTrackFLL = "Stopa PLL"; // front left track, left offset
    memoryPointTrackFLR = "Stopa PLP"; // front left track, right offset
    memoryPointTrackBLL = "Stopa ZLL"; // back left track, left offset
    memoryPointTrackBLR = "Stopa ZLP"; // back left track, right offset
    memoryPointTrackFRL = "Stopa PPL"; // front right track, left offset
    memoryPointTrackFRR = "Stopa PPP"; // front right track, right offset
    memoryPointTrackBRL = "Stopa ZPL"; // back right track, left offset
    memoryPointTrackBRR = "Stopa ZPP"; // back right track, right offset

    memoryPointCirculumReference="circulumReference";  // center of turning of whole vehicle

    //the values are ratios, 3 values are at least required {reverse, neutral, 1st gear}
    gearBox[]={-8,0,+10,+6.15,+4.44,+3.33};

    Scudeffect = "ScudEffect";
    dammageHalf[]= {};
    dammageFull[]= {};

    armorLights=0.4;

    vehicleClass="Car";

    wheelCircumference=2.513;
    turnCoef=3.0;
    terrainCoef=3.0;
    waterSpeedFactor=0.2;
    maxSpeed=100;

    preferRoads=true; // never used value, engine don't use it
    unitInfoType=RscUnitInfo;
    hideUnitInfo=true;

    formationX=20;
    formationZ=20;

    precision=10;
    brakeDistance=10; // vehicle movement precision

    steerAheadSimul=0.5; // steer point on path properties
    steerAheadPlan=0.35;

    predictTurnSimul=1.2; // brake before turns
    predictTurnPlan=1.2;

    sensitivity=3; // sensor sensitivity

    soundGear[] = {"", db-75, 1};

    extCameraPosition[]={0,1,-10};

    // internal camera viewing limitations
    class ViewPilot: ViewPilot
    {
      initFov=1.1; minFov=0.57; maxFov=1.4;
      initAngleX=15; minAngleX=-85; maxAngleX=+85; //Rg 15/25;
      initAngleY=0; minAngleY=-150; maxAngleY=+150; //Rg 100;
    };

    simulation=car;

    weapons[]={};
    magazines[]={};
    type=VSoft;
    //threat[] VSoft, VArmor, VAir
    threat[]={0.5, 0.5, 0.5};

    typicalCargo[]={Soldier};

    audible = 3;

    occludeSoundsWhenIn = db-20;
    obstructSoundsWhenIn = db-10;

    getInAction = "";
    getOutAction = "";

    // by default there is no SCUD rocket
    scudModel = "";
    damperSize = 0.1; // max. damper amplitude
    damperForce = 1; // damper reaction force (bigger is more visible)
    damperDamping = 1; // schock absorbers bigger => more tough

    inputTurnCurve[] = {{0, {0,0,1,1}},{ 30, { 0, 0, 0.2, 0.008, 0.4, 0.032, 0.6, 0.216, 0.8, 0.512, 1, 1}}};

    enableGPS = false;

    soundEngine[]={"",db5,0.9};
    soundEnviron[] = {"", db-5, 1};

    class Sounds: Sounds
    {
      class Engine: Engine
      {
        sound = "soundEngine";
        frequency = "(randomizer*0.05+0.95)*rpm*1.2";
        volume = "thrust*0.5+0.5";
      };
      class Movement: Movement
      {
        sound = "soundEnviron";
        frequency = "1";
        volume = "speed*0.03334";
      };
    };

  };

  class Motorcycle: LandVehicle
  {
    //--
    simulation="motorcycle";
    displayName=$STR_DN_MOTORCYCLE;
    accuracy=0.20;

    mapSize = 5;

    icon = iconMotorcycle;
    nameSound="veh_car";

    isBicycle = false;

    unloadInCombat = true;

    ejectDeadDriver = true;
    ejectDeadCargo = true;

    armor=10; // total armor
    armorStructural=4.0; // very low structural dammage
    cost=500;
    fuelCapacity=50;

    // Damper configuration
    damperSize = 0.1;
    damperForce = 1; // larger number more stiffness dampers
    damperDamping = 3; // larger number less movement in dampers

    transportSoldier = 0;
    typicalCargo[]={};

    occludeSoundsWhenIn = db-0;
    obstructSoundsWhenIn = db-0;

    driverCanSee = CanSeeEar+CanSeeEye+CanSeePeripheral;
    gunnerCanSee = CanSeeEye+CanSeeEar+CanSeePeripheral;

    class PlateInfos {name="spz"; /*font<=fontPlate;*/ color[]={0,0,0,0.75};}

    class HitPoints
    {
      class HitEngine {armor=1.2;material=60;name=engine;visual="engine";passThrough=1;}

      class HitRGlass {armor=0.5;material=-1;name="sklo predni P";convexComponent="sklo predni P";visual="";passThrough=1;}
      class HitLGlass {armor=0.5;material=-1;name="sklo predni L";convexComponent="sklo predni L";visual="";passThrough=1;}

      class HitBody {armor=0.4;material=-1;name="karoserie";visual="karoserie";passThrough=1;}
      class HitFuel {armor=1.4;material=-1;name="palivo";visual="palivo";passThrough=1;}

      class HitFWheel {armor=0.05;material=-1;name="Pravy predni tlumic";visual="Pravy predni";passThrough=1;}
      class HitBWheel {armor=0.05;material=-1;name="Pravy zadni tlumic";visual="Pravy zadni";passThrough=1;}
    };

    selectionFireAnim = "zasleh";

    alphaTracks = 0.1;
    textureTrackWheel = true;
    memoryPointTrack1L = "Stopa PPL"; // front track, left offset
    memoryPointTrack1R = "Stopa PPP"; // front track, right offset
    memoryPointTrack2L = "Stopa ZPL"; // back track, left offset
    memoryPointTrack2R = "Stopa ZPP"; // back track, right offset

    vehicleClass="Car";

    wheelCircumference=2.513;
    turnCoef=1.0;
    terrainCoef=3.0;
    maxSpeed=120;

    gearBox[]={-8,0,+10,+6.15,+4.44,+3.33};

    preferRoads=true; // never used value, engine don't use it
    unitInfoType=RscUnitInfo;
    hideUnitInfo=true;

    formationX=20;
    formationZ=20;

    precision=10;
    brakeDistance=25; // vehicle movement precision

    steerAheadSimul=0.5; // steer point on path properties
    steerAheadPlan=0.35;

    predictTurnSimul=1.2; // brake before turns
    predictTurnPlan=1.2;

    sensitivity=3; // sensor sensitivity

    soundGear[] = {"", db-100, 1};

    extCameraPosition[]={0,1,-10};

    // internal camera viewing limitations
    class ViewPilot: ViewPilot
    {
      initFov=1; minFov=0.57; maxFov=1.4;
      initAngleX=15; minAngleX=-85; maxAngleX=+85; //Rg 15/25;
      initAngleY=0; minAngleY=-150; maxAngleY=+150; //Rg 100;
    };

    class Reflectors
    {
      class Right
      {
        color[] = {0.9, 0.8, 0.8, 1.0};
        ambient[] = {0.1, 0.1, 0.1, 1.0};
        position = "P svetlo";
        direction = "konec P svetla";
        hitpoint = "P svetlo";
        selection = "P svetlo";
        size = 0.5;
        brightness = 0.5;
      };
    };

    weapons[]={};
    magazines[]={};
    type=VSoft;
    //threat[] VSoft, VArmor, VAir
    threat[]={0.5, 0.5, 0.5};

    audible = 3;

    getInAction = "";
    getOutAction = "";

    class AnimationSources
    {
      class FrontDamper
      {
        source="damper";
        selection="pravy predni tlumic nerot"; // landcontact selection name
      };
      class BackDamper
      {
        source="damper";
        selection="pravy zadni tlumic"; // landcontact selection name
      };
    };
    // Animations moved to model.cfg
  };
  class Bicycle: Motorcycle
  {
    canFloat = false;
    isBicycle = true;
    hideWeaponsDriver = false;

    formationX=5;
    formationZ=5;

    precision=5;
    brakeDistance=10; // vehicle movement precision

    steerAheadSimul=0.5; // steer point on path properties
    steerAheadPlan=0.7;

    predictTurnSimul=1.2; // brake before turns
    predictTurnPlan=1.2;
  };


  class Tank: LandVehicle
  {
    //--
    vehicleClass="Armored";

    icon = iconTank;

    displayName=$STR_DN_TANK;
    nameSound="veh_tank";
    accuracy=0.12;

    getInRadius = 3.5;


    fuelCapacity=700;

    irScanRangeMin=500;
    irScanRangeMax=4000;
    irScanToEyeFactor=1;

    armor=400; // overall armor
    armorStructural=2.0; // structural dammage
    crewVulnerable = false;

    class HitPoints
    {
      class HitEngine {armor=0.8;material=60;name=engine;visual="engine";passThrough=1;}

      class HitHull {armor=1;material=50;name=hull;visual="hull";passThrough=1;};
      class HitLTrack {armor=0.6;material=53;name=pasL;visual="pasL";passThrough=1;};
      class HitRTrack {armor=0.6;material=54;name=pasP;visual="pasP";passThrough=1;};
    };


    selectionFireAnim = "zasleh";

    memoryPointCargoLight = "cargo light";

    // selection used to extend bounding sphere / bounding box as necessary
    bounding = "usti hlavne";
    fireDustEffect = "FDustEffects";

    gearBox[]={-7,0,+11,+8,+5.7,+4.2};

    alphaTracks = 0.7;
    textureTrackWheel = false;
    memoryPointTrack1L = "Stopa LL"; // left track, left offset
    memoryPointTrack1R = "Stopa LR"; // left track, right offset
    memoryPointTrack2L = "Stopa RL"; // right track, left offset
    memoryPointTrack2R = "Stopa RR"; // right track, right offset

    extCameraPosition[]={0,1.5,-9};

    //initFov=0.7; minFov=0.42; maxFov=0.85;
    class ViewPilot: ViewPilot
    {
      initAngleX=7; minAngleX=-15; maxAngleX=+25;
      initAngleY=0; minAngleY=-90; maxAngleY=+90;
    };

    class ViewOptics : ViewOptics
    {
      initFov=0.3; minFov=0.07; maxFov=0.35;
    };

    cost=1000000;

    steerAheadSimul=0.4; // steer point on path properties
    steerAheadPlan=0.6;

    predictTurnSimul=1.2; // brake before turns
    predictTurnPlan=1.8;

    // default tank/bmp... track vehicle definition
    soundGear[] = {"", db-70, 1};

    occludeSoundsWhenIn = db-40;
    obstructSoundsWhenIn = db-15;
    outsideSoundFilter = true;

    nightVision=false;

    driverAction = "";
    driverInAction = "";

    //transportSoldier = 2;

    simulation=tank;

    formationX=20;
    formationZ=30;
    precision=5;
    brakeDistance=15; // vehicle movement precision

    maxSpeed=80;

    /// how fast tracks are moving relative to wheels
    tracksSpeed = 1;
    turnCoef = 1;

    class Turrets
    {
      class MainTurret: NewTurret
      {
        commanding = true;

        class Turrets
        {
          class CommanderOptics: NewTurret
          {
            gunnerName = $STR_POSITION_COMMANDER;
            primaryGunner = 0;
            primaryObserver = true;
            body = "obsTurret";
            gun = "obsGun";
            animationSourceBody = "obsTurret";
            animationSourceGun = "obsGun";
            animationSourceHatch = "hatchCommander";
            commanding = 2;
          };
        };
      };
    };

    canFloat=false;
    weapons[]={FakeWeapon};
    magazines[]={FakeWeapon};
    type=VArmor;
    //threat[] VSoft, VArmor, VAir
    threat[]={0.7, 1, 0.3};

    //sensors
    camouflage = 8; // tank are very easy to spot
    audible = 6;

    hideProxyInCombat = true;

    driverOpticsModel = "optika_tank_driver";

    driverOpticsColor[] = {1, 1, 1, 1};

    class CargoLight
    {
      color[] = {0, 0, 0, 0};
      ambient[] = {0.6, 0, 0.15, 1};
      brightness = 0.007;
    };

    enableGPS = true;

    class Sounds: Sounds
    {
      class Engine: Engine
      {
        sound = "soundEngine";
        frequency = "(1-(randomizer*0.05))*rpm*1.2";
        volume = "thrust*0.5+0.5";
      };
      class Movement: Movement
      {
        sound = "soundEnviron";
        frequency = "(speed+angVelocity)*0.03334";
        volume = "((speed+angVelocity)*0.03334)+(1-((speed+angVelocity)*0.03334))*0.3";
      };
    };
  };
  class APC: Tank
  {
    //--
    icon = iconAPC;

    vehicleClass="Armored";

    displayName=$STR_DN_APC;
    nameSound="veh_apc";
    accuracy=0.12;

    fuelCapacity=700;
    armor=200;
    cost=1000000;

    // default tank/bmp... track vehicle definition
    soundGear[] = {"", db-60, 1};

    simulation=tank;

    maxSpeed=60;

    // new style of turrets definition - structure description
    // must be redefined here to let MainTurret inherit from the correct class
    class Turrets: Turrets
    {
      class MainTurret : MainTurret
      {
        minElev=-4.5;
        maxElev=+20;
      };
    };

    canFloat=false;
    weapons[]={FakeWeapon};
    magazines[]={FakeWeapon};
    type=VArmor;
    //threat[] VSoft, VArmor, VAir
    threat[]={0.8, 1, 0.3};
  };

  class Man: Land
  {
    //--
    mapSize = 0.5;
    vehicleClass="Men";

    icon = iconMan;
    autocenter = false; // no autocenter - faster animation
    isMan = true;

    displayName=$STR_DN_MAN;
    nameSound="veh_man";

    // subclass of CfgWorlds.GenericNames, used for names
    genericNames = "EnglishMen";

    clothingType = "male";

    extCameraPosition[]={0,1.0,-3.5};

    accuracy=0.25;
    /*!
    \patch 1.27 Date 10/18/2001 by Ondra
    - Fixed: soldier movement was heard too far by AI.
    */
    audible=0.05;
    camouflage=1; // how easy to spot -> bigger means better spotable

    sensitivity=3;
    sensitivityEar=0.13;

    class visibility
    {
      down = 0.1;
      prone = 0.1;
      kneel = 0.5;
      stand = 1.0;
    };
    
    class audibility
    {
      down = 0.1;
      prone = 0.1;
      kneel = 0.5;
      stand = 1.0;
    };

    formationX=5;
    formationZ=5;
    precision=1;
    brakeDistance=1; // vehicle movement precision

    steerAheadSimul=0.1; // steer point on path properties
    steerAheadPlan=0.1;

    predictTurnSimul=0.2; // brake before turns in buildings
    predictTurnPlan=0.2;

    maxSpeed=24;
    /// the limit for AI turn speed in rad/sec
    maxTurnAngularVelocity = 3;
    costTurnCoef = 0.025;

    lyingLimitSpeedHiding = 0.8;
    crouchProbabilityHiding = 0.8;
    lyingLimitSpeedCombat = 1.8;
    crouchProbabilityCombat = 0.4;
    lyingLimitSpeedStealth = 2;

    irTarget=false; // no ir lock possible
    canHideBodies=false;
    canDeactivateMines=false;

    useInternalLODInVehicles = true;

    unitInfoType=RscUnitInfoSoldier;
    hideUnitInfo=true;

    armor=3;
    armorStructural=2.0; // structural dammage

    class HitPoints
    {
      // note: use visual="", men use Wounds instead
      class HitHead {armor=0.7;material=-1;name="hlava_hit";visual="";passThrough=1;}
      class HitBody {armor=0.8;material=-1;name="telo";visual="";passThrough=1;}
      class HitHands {armor=0.5;material=-1;name="ruce";visual="";passThrough=1;}
      class HitLegs {armor=0.5;material=-1;name="nohy";visual="";passThrough=1;}
    };

    cost=100000;
    simulation=soldier;

    // by default there is nothing to talk about
    class TalkTopics {};

    /// texture/material animation based on wounding
    class Wounds {};

    minGunElev=-60;maxGunElev=+60; // + up
    /*!
    \patch 1.43 Date 01/22/2002 by Viktor
    - Changed: Soldiers can aim more down from now
    */

    minGunTurn=-5;maxGunTurn=+5; // + left - player
    //minGunTurn=-30;maxGunTurn=+30; // + left - player
    minGunTurnAI=-30;maxGunTurnAI=+30; // + left - AI

    commanderCanSee = CanSeeAll+CanSeePeripheral;

    class HeadLimits: HeadLimits
    {
      initAngleX=5; minAngleX=-40; maxAngleX=+40;
      initAngleY=0; minAngleY=-90; maxAngleY=+90;
    };
    class ViewPilot: ViewPilot
    {
      initFov=0.95; minFov=0.4; maxFov=1.1;
      initAngleX=8; minAngleX=-85; maxAngleX=+85; //Rg 40/45;
      initAngleY=0; minAngleY=-150; maxAngleY=+150; //Rg 125;
    };
    microMimics = Micro; // see CfgMimics/Micro

    minHeadTurnAI = -70;
    maxHeadTurnAI = +70;

    weaponSlots = WeaponSlotPrimary + 5 * WeaponSlotItem + WeaponSlotBinocular + WeaponSlotHandGun + 4*WeaponSlotHandGunItem + 12* WeaponSlotInventory;
    weapons[]={};
    magazines[]={};
    model=;
    picture=;

    boneHead = "hlava";
    boneHeadCutScene = "hlava";

    /// List of identifiers for selection of identity items (faces, voices)
    identityTypes[] = {"Default"};

    // by default, glasses can be shown
    glassesEnabled = 1;

    // RTM animation responsible for trigger pulling finger movement
    triggerAnim = "";

    selectionHeadWound = "head injury";
    selectionBodyWound = "body injury";
    selectionLArmWound = "l arm injury";
    selectionRArmWound[] = {"r arm injury", "p arm injury"};
    selectionLLegWound = "l leg injury";
    selectionRLegWound[] = {"r leg injury", "p leg injury"};
    selectionHeadHide = "";

    memoryPointPilot = "pilot";
    memoryPointLStep = "stopaL";
    memoryPointRStep = "stopaP";
    memoryPointAim = "zamerny";
    memoryPointCameraTarget = "zamerny";
    memoryPointAimingAxis = "osa mireni";
    memoryPointLeaningAxis = "osa naklaneni";
    memoryPointHeadAxis = "osa otaceni";
    memoryPointHandGrenade = "granat";

    memoryPointCommonDamage = "zamerny";

    // head selections
    selectionPersonality = "osobnost";
    selectionGlasses = "brejle";

    // mimic
    selectionLBrow = "loboci";
    selectionMBrow = "soboci";
    selectionRBrow = "poboci";
    selectionLMouth = "lkoutek";
    selectionMMouth = "skoutek";
    selectionRMouth = "pkoutek";
    selectionEyelid = "vicka";
    selectionLip = "spodni ret";

    // say something when hit
    hitSound1[] = {"", db-25, 1};
    hitSound2[] = {"", db-25, 1};
    hitSound3[] = {"", db-25, 1};
    hitSound4[] = {"", db-25, 1};
    hitSound5[] = {"", db-25, 1};
    hitSound6[] = {"", db-25, 1};
    hitSound7[] = {"", db-25, 1};
    hitSound8[] = {"", db-25, 1};
    hitSound9[] = {"", db-25, 1};
    hitSound10[] = {"", db-25, 1};
    hitSound11[] = {"", db-25, 1};
    hitSound12[] = {"", db-25, 1};
    hitSound13[] = {"", db-25, 1};
    hitSound14[] = {"", db-25, 1};
    hitSound15[] = {"", db-25, 1};
    hitSound16[] = {"", db-25, 1};
    hitSound17[] = {"", db-25, 1};
    hitSound18[] = {"", db-25, 1};
    hitSound19[] = {"", db-25, 1};
    hitSound20[] = {"", db-25, 1};

    hitSounds[]=
    {
      hitSound1,0.05,
      hitSound2,0.05,
      hitSound3,0.05,
      hitSound4,0.05,
      hitSound5,0.05,
      hitSound6,0.05,
      hitSound7,0.05,
      hitSound8,0.05,
      hitSound9,0.05,
      hitSound10,0.05,
      hitSound11,0.05,
      hitSound12,0.05,
      hitSound13,0.05,
      hitSound14,0.05,
      hitSound15,0.05,
      hitSound16,0.05,
      hitSound17,0.05,
      hitSound18,0.05,
      hitSound19,0.05,
      hitSound20,0.05
    };

    moves=NoDefaultMoves;
    gestures="";

    emptySound[]={"",0,1};
    vegetationSounds[]={emptySound, 0};

    additionalSound[] = {"", db-120, 1};
    class SoundEnvironExt
    {
      normalExt[] = {{"", db-100, 1, 0}};
      normal[] = {{"", db-100, 1, 0}};
      road[] = {{"", db-100, 1, 0}};
      rock[] = {{"", db-100, 1, 0}};
      water[] = {{"", db-100, 1, 0}};
      gravel[] = {{"", db-100, 1, 0}};
      sand[] = {{"", db-100, 1, 0}};
      drygrass[] = {{"", db-100, 1, 0}};
      grass[] = {{"", db-100, 1, 0}};
      forest[] = {{"", db-100, 1, 0}};
      mud[] = {{"", db-100, 1, 0}};
      wood[] = {{"", db-100, 1, 0}};
      metal[] = {{"", db-100, 1, 0}};
      snow[] = {{"", db-100, 1, 0}};
      hallway[] = {{"", db-100, 1, 0}};
      fallbody[] = {{"", db-100, 1, 0}};
      laydown[] = {{"", db-100, 1, 0}};
      standup[] = {{"", db-100, 1, 0}};
      crawl[] = {{"", db-100, 1, 0}};
    };

    class SoundEquipment
    {
      civilian[] = {{"", {"", db-100, 1, 0}}};
      soldier[] = {{"", {"", db-100, 1, 0}}};
    };

    class SoundGear
    {
      primary[] = {{"",{"", db-100, 1, 0}}};
      secondary[] = {{"",{"", db-100, 1, 0}}};
    };

    class SoundBreath
    {
      breath[] = {};//{{{"", db-100, 1}, 1}, {{"", db-100, 1}, 1}};
    };

    class HitDamage {};

    type=VSoft;
    //threat[] VSoft, VArmor, VAir
    threat[]={1, 0.8, 0.2};

    class Reflectors
    {
      /*
      class Reflector
      {
      color[] = {0.9, 0.8, 0.8, 1.0};
      ambient[] = {0.1, 0.1, 0.1, 1.0};
      position = "L svetlo";
      direction = "konec L svetla";
      hitpoint = "L svetlo";
      selection = "L svetlo";
      size = 0.2;
      brightness = 0.001;
      };
      */
    };
    class MGunClouds : WeaponCloudsMGun
    {
    };
    class GunClouds : WeaponCloudsGun
    {
      cloudletGrowUp = 0.2;     // grow-up time
      cloudletFadeIn = 0;   // fade-in time
      cloudletFadeOut = 0.4;    // fade-out time
      cloudletDuration = 0.2;   // time to live (not including fades)

      cloudletAlpha = 1;

      cloudletAccY = 2;         // vertical acceleration
      cloudletMinYSpeed = -10;  // vertical speed range
      cloudletMaxYSpeed = 10;

      interval = 0.02;
      size = 0.3;
      sourceSize = 0.015;
    };

    class InventorySlots
    {
    };

    fsmFormation = "";
    fsmDanger = "";
    //    fsmDanger = "bin\danger.fsm";

    enableGPS = false;

	leftArmToElbow[] = {}; // jmeno kosti, pomìr rotace
	leftArmFromElbow[] = {};
	leftWrist = ""; // jmeno kosti
	leftShoulder = ""; // jmeno kosti
	leftHand[] = {}; // jmeno kosti
	leftArmPoints[] = {"","","",""}; // jmena bodu v memory lod, definujicich rozsahy kosti


	rightArmToElbow[] = {}; // jmeno kosti, pomìr rotace
	rightArmFromElbow[] = {};
	rightWrist = ""; // jmeno kosti
	rightShoulder = ""; // jmeno kosti
	rightHand[] = {}; // jmeno kosti
	rightArmPoints[] = {"","","",""}; // jmena bodu v memory lod, definujicich rozsahy kosti

  leftLegToKnee[] = {}; 
  leftLegFromKnee[] = {};
  leftHeel = "";
  leftHip = ""; 
  leftFoot[] = {}; 
  leftLegPoints[] = {"", "", "", ""}; 

  rightLegToKnee[] = {}; 
  rightLegFromKnee[] = {};
  rightHeel = ""; 
  rightHip = "";
  rightFoot[] = {}; 
  rightLegPoints[] = {"", "", "", ""}; 

	weaponBone = "";
	launcherBone = "";
	handGunBone = "";

	characterID = -1;

    // defaults for empty slots
    emptyEyewear = "";
    emptyHeadgear = "";
    emptyBody = "";
    emptyLegs = "";
    emptyFeet = "";
    emptyVest = "";
    emptyGloves = "";
    emptyBack = "";
    emptyShoulder = "";
    emptyShoulder2 = ""; // optional second large-weapon slot
    emptyMelee = "";
    emptyMask = "";
  };

  /// Base class for agent based animals
  class Animal : Man
  {
    vehicleClass = "Animals";

    icon = iconAnimal;
    isMan = false;

    displayName = $STR_DN_ANIMAL;
    nameSound="veh_animal";

    accuracy=0.25;

    weaponSlots = 0;
    weapons[]={};
    magazines[]={};

    type=VSoft;
    //threat[] VSoft, VArmor, VAir
    threat[]={0, 0, 0};
  };

  /*!
  \patch 1.52 Date 4/21/2002 by Ondra
  - Fixed: AI perceived volume of helicopters and planes was too low.
  */

  class Air: AllVehicles
  {
    //-- air vehicle
    icon = iconAir;
    displayName=$STR_DN_HELICOPTER; // assume heli until sure it's plane
    nameSound="veh_aircraft";
    accuracy=0.005;

    formationX=50;
    formationZ=100;
    precision=100;
    brakeDistance=200; // vehicle movement precision
    formationTime=10;

    armor=20;
    cost=10000000;
    fuelCapacity=1000;

    maxSpeed=400;

    insideSoundCoef = 0.5;
    outsideSoundFilter = true;
    occludeSoundsWhenIn = db-30;
    obstructSoundsWhenIn = db-15;

    weapons[]={FakeWeapon};
    magazines[]={FakeWeapon};

    type=VAir;
    /**
    Determine the ir scanner / radar range based on visibility.
    range is computed as visibility*irScanToEyeFactor and then clamped by min/max range.

    This should allow providing realistic values for high visibilities while still allowing
    to somewhat decrease the radar range for low visibility for gameplay balancing,
    or it can be used to simulated devices which are linked to visibility (IR trackers)
    */
    irScanRangeMin = 2000;
    irScanRangeMax = 10000;
    irScanToEyeFactor = 2;
    nightVision=false;
    audible = 10; // planes are very loud

    unitInfoType=RscUnitInfoAir;

    //threat[] VSoft, VArmor, VAir
    threat[]={0.3, 1, 0.7};

    driverAction = "";

    driverCompartments = 0; // no switching positions for planes and helicopters
    cargoCompartments[] = {0};

    gunnerCanSee = CanSeeAll+CanSeePeripheral;
    driverCanSee = CanSeeAll+CanSeePeripheral;

    typicalCargo[]={Soldier};

    getInRadius = 5;

    enableGPS = true;

    class MarkerLights
    {
      class RedStill
      {
        name = "cerveny pozicni";
        color[] = {0.3, 0.03, 0.03, 1};
        ambient[] = {0.03, 0.003, 0.003, 1};
        brightness = 0.01;
        blinking = false;

      };
      class GreenStill
      {
        name = "zeleny pozicni";
        color[] = {0.03, 0.3, 0.03, 1};
        ambient[] = {0.003, 0.03, 0.003, 1};
        brightness = 0.01;
        blinking = false;
      };
      class WhiteStill
      {
        name = "bily pozicni";
        color[] = {0.3, 0.3, 0.3, 1};
        ambient[] = {0.03, 0.03, 0.03, 1};
        brightness = 0.01;
        blinking = false;
      };
      class WhiteBlinking
      {
        name = "bily pozicni blik";
        color[] = {1.0, 1.0, 1.0, 1};
        ambient[] = {0.1, 0.1, 0.1, 1};
        brightness = 0.01;
        blinking = true;
      };
      class RedBlinking
      {
        name = "cerveny pozicni blik";
        color[] = {0.5, 0.05, 0.05, 1};
        ambient[] = {0.05, 0.005, 0.005, 1};
        brightness = 0.01;
        blinking = true;
      };
      /*
      class GreenBlinking
      {
        name = "zeleny pozicni blik";
        color[] = {0.1, 1.0, 0.1, 1};
        ambient[] = {0.01, 0.1, 0.01, 1};
        brightness = 0.01;
        blinking = true;
      };
      class BlueBlinking
      {
        name = "modry pozicni blik";
        color[] = {0.1, 0.1, 1.0, 1};
        ambient[] = {0.01, 0.01, 0.1, 1};
        brightness = 0.01;
        blinking = true;
      };
      */
    };
  };
  class Helicopter: Air
  {
    //-- helicopter
    vehicleClass="Air";
    icon = iconHelicopter;

    displayName=$STR_DN_HELICOPTER;
    nameSound="veh_helicopter";
    accuracy=0.08;

    simulation=helicopter;
    gearRetracting  = false;

    dustEffect = "HeliDust";
    waterEffect = "HeliWater";

    damageEffect = "DamageSmokeHeli";


    gearUpTime = 3.33;
    gearDownTime = 2.0;

    class ViewPilot: ViewPilot
    {
      initFov=1.1; minFov=0.57; maxFov=1.1;
      initAngleX=10; minAngleX=-85; maxAngleX=+85; //Rg 15/25;
      initAngleY=0; minAngleY=-150; maxAngleY=+150; //Rg 100;
      // CockpitHeadMovement parameters
      minMoveX=-0.35; maxMoveX=0.35; //how far can the head be moved to the sides Left and Right
      minMoveY=-0.5; maxMoveY=0.2; //how far can the head be moved Down and Up
      minMoveZ=-0.3; maxMoveZ=0.5; //how far can the head be moved Backward and Forward
    };
    class CargoSpec
    {
      class Cargo1
      {
        showHeadPhones = true;
      };
      class Cargo2
      {
        showHeadPhones = false; // only first cargo position with earPhones as default
      };
    };

    mainRotorSpeed = 1.0; // rotor animation parameters
    backRotorSpeed = 1.5;

    maxMainRotorDive = 0; // rotor diving capability
    maxBackRotorDive = 0;
    minMainRotorDive = 0; // rotor diving capability
    minBackRotorDive = 0;
    neutralBackRotorDive = 0;
    neutralMainRotorDive = 0;

    selectionHRotorStill = "velka vrtule staticka";
    selectionHRotorMove = "velka vrtule blur";
    selectionVRotorStill = "mala vrtule staticka";
    selectionVRotorMove = "mala vrtule blur";
    memoryPointLMissile = "L strela";
    memoryPointRMissile = "P strela";
    memoryPointLRocket = "L raketa";
    memoryPointRRocket = "P raketa";
    memoryPointGun = "";
    memoryPointPilot = "pilot"; // also searching in VIEW_PILOT

    selectionFireAnim = "zasleh";
    
    class HitPoints
    {
      class HitHull {armor=0.5;material=50;name="trup";visual="trup";passThrough=1;}
      class HitEngine {armor=0.6;material=60;name="motor";visual="motor";passThrough=1;}
      class HitAvionics {armor=1.4;material=-1;name="elektronika";visual="elektronika";passThrough=1;}
      class HitVRotor {armor=0.5;material=52;name="mala vrtule";visual="mala vrtule";passThrough=1;}
      class HitHRotor {armor=0.7;material=51;name="velka vrtule";visual="velka vrtule";passThrough=1;}
      class HitMissiles {armor=1.6;material=-1;name="munice";visual="";passThrough=1;}
      class HitRGlass {armor=0.5;material=-1;name="sklo predni P";convexComponent="sklo predni P";visual="";passThrough=1;}
      class HitLGlass {armor=0.5;material=-1;name="sklo predni L";convexComponent="sklo predni L";visual="";passThrough=1;}
    };

    armorStructural=2.0; // structural dammage

    maxSpeed=300;
    enableSweep=true;

    /// lift (G) based on speed
    envelope[]=
    {
      // speed relative to max. speed -> lift
      //0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0 1.1 1.2 1.3 1.4  // rel. speed
      0.0,0.2,0.9,2.1,2.5,3.3,3.5,3.6,3.7,3.8,3.8,3.0,0.9,0.7,0.5 // lift
    };

    unitInfoType=RscUnitInfoAir;

    minFireTime=20; // minimal time spent firing on single target

    steerAheadSimul=0.5;
    steerAheadPlan=0.7;

    // new style of turrets definition - structure description
    class Turrets
    {
      class MainTurret : NewTurret
      {
        outGunnerMayFire = true; //Engine assumes helicopter gunner is turned out.
        commanding = -1;

        //gunnerCompartments = 0;
        startEngine = 0;

        class TurretSpec : TurretSpec
        {
          showHeadPhones = true;
        };
      };
    };

    class ViewOptics : ViewOptics
    {
      initAngleX=0; minAngleX=-40; maxAngleX=17;
      initAngleY=0; minAngleY=-100; maxAngleY=100;
      initFov=0.5; minFov=0.3; maxFov=1.2;
    };
    class MFD
    {
      class HUD: AirplaneHUD
      {
      };
    };

    weapons[]={FakeWeapon};
    magazines[]={FakeWeapon};

    dammageHalf[]=
    {
      jeep_kab_sklo1.paa,jeep_kab_sklo1B.paa,
      uh60_kab_sklo2.paa,uh60_kab_sklo2B.paa,
      uh60_gunner_okna.paa,uh60_gunner_oknaB.paa,
      ah-1_kabina_predokno.paa,ah-1_kabina_predoknoB.paa,
      ah-1_kabina_zadokno.paa,ah-1_kabina_zadoknoB.paa,
      ah-1_kabina_topokno.paa,ah-1_kabina_topoknoB.paa
    };
    dammageFull[]=
    {
      jeep_kab_sklo1.paa,jeep_kab_sklo1C.paa,
      uh60_kab_sklo2.paa,uh60_kab_sklo2C.paa,
      uh60_gunner_okna.paa,uh60_gunner_oknaC.paa,
      ah-1_kabina_predokno.paa,ah-1_kabina_predoknoC.paa,
      ah-1_kabina_zadokno.paa,ah-1_kabina_zadoknoC.paa,
      ah-1_kabina_topokno.paa,ah-1_kabina_topoknoC.paa
    };

    class Reflectors
    {
      class Reflector
      {
        color[] = {0.8, 0.8, 1.0, 1.0};
        ambient[] = {0.07, 0.07, 0.07, 1.0};
        position = "L svetlo";
        direction = "konec L svetla";
        hitpoint = "L svetlo";
        selection = "L svetlo";
        size = 0.5;
        brightness = 2;
      };
    };

    class Sounds: Sounds
    {
      class Engine: Engine
      {
        sound = "soundEngine";
        frequency = "(1-randomizer*0.05)*rotorSpeed*(1-rotorThrust*0.1)";
        volume = "1";
      };
      class Movement: Movement
      {
        sound = "soundEnviron";
        frequency = "1";
        volume = "speed * 0.03334";
      };
    };

    soundLandingGear[] = {"", 1, 1};
    
    class Exhausts
    {
    };

    class RotorLibHelicopterProperties
    {
      RTDconfig = "";     // config for RotorLib simulation
      hasAPU = false;     //equipped with APU     
      maxTorque = 10000;  //maximum safe torque

      //battery configuration
      maxBatteryCharge = 100.0;
      starterBatteryDrain = 2.0;
      electricityBatteryDrain = 0.1;
      engineBatteryRecharge = 0.5;
      //for some helicopters pitch = roll = 0 is not stable orientation
      autoHoverCorrection[] = {0,0,0};

      //maximum stress levels
      maxMainRotorStress = 10000;
      maxTailRotorStress = 10000;
      maxHorizontalStabilizerLeftStress = 10000;
      maxHorizontalStabilizerRightStress = 10000;
      maxVerticalStabilizerStress = 10000;
      stressDamagePerSec = 0.01; //damage per one second when max. stress level is reached (if stress is 150%, damage is (1.5^2)*timeToBreakUP per sec.)

      //throttle off  {heating, decrease per second (* 0.1f if 0 RPM), cooling (with Starter), decrease per second (RPM > 0)}
      //throttle idle {heating, increase per second, cooling, decrease per second}
      //throttle full {heating, increase per second, cooling, decrease per second}
      temperatureEffects[] = {{0, -25, 0, -15},{850, 250, -400, -20},{1350, 250, -600, -20}};
      maxEngineTemperature = 1000; //temperature over maxEngineTemperature causes explosion 
      hotStartTemperature = 200;
      retreatBladeStallWarningSpeed = 69.4; // m/s

      //collective based horizontal wing rotation (degrees)
      horizontalWingsAngleCollMin = 0;  // collective 0
      horizontalWingsAngleCollMax = 0;  // collective 1

      //collective when flying heli is placed in world
      defaultCollective = 0.7;
    };
    /*
    hRotorAnimation = HRotor;
    vRotorAnimation = VRotor;
    */
  };
  class Plane: Air
  {
    //-- airplane
    vehicleClass="Air";

    damageEffect = "DamageSmokePlane";

    icon = iconPlane;

    displayName=$STR_DN_PLANE;
    nameSound="veh_airplane";
    accuracy=0.10;

    maxSpeed=450;
    landingSpeed = 0;
    flapsFrictionCoef = 0.5;

    formationX=200;
    formationZ=300;
    precision=200;
    brakeDistance=500; // vehicle movement precision
    steerAheadSimul=1.0;
    steerAheadPlan=2.0;
    wheelSteeringSensitivity = 1.0;

    unitInfoType=RscUnitInfoAir;
    gearRetracting = true;
    cabinOpening = true;
    flaps = true;
    airBrake = true;
    vtol=false;
    // reflector is most often located on the landing gear and needs to be off when gear is up
    lightOnGear=true;

    gearUpTime = 3.33;
    gearDownTime = 2.0;

    ejectSpeed[]={0,40,0};
    /// units will eject once the plane is damage too much
    ejectDamageLimit = 0.45;

    minFireTime=60; // minimal time spent firing on single target

    cost=2000000;
    simulation=airplane;

    minGunElev=-0; // default - fixed gun
    maxGunElev=+0;
    minGunTurn=-0;
    maxGunTurn=+0;

    /// gun down aim angle as percentile
    gunAimDown = 0; // default gun aims forwards

    /// lift (G) based on speed
    envelope[]=
    {
      // speed relative to max. speed -> lift
      // 0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0 1.1 // rel. speed
      0.0,0.2,0.9,2.1,2.5,3.3,3.5,3.2,2.5,2.0,1.5,1.0 // lift
    };
    weapons[]={FakeWeapon};
    magazines[]={FakeWeapon};

    type=VAir;
    //threat[] VSoft, VArmor, VAir
    threat[]={0.1, 1, 0.5};

    aileronSensitivity = 1; // relative aileron sensitivity
    elevatorSensitivity = 1; // relative elevator sensitivity
    // noseDownCoef is no longer used

    // angle of attack recommended for AI / landing autopilot
    landingAoa = 10*3.1415/180;

    // internal camera viewing limitations
    class ViewPilot: ViewPilot
    {
      initFov=1; minFov=0.57; maxFov=1.4;
      initAngleX=6; minAngleX=-85; maxAngleX=+85; //Rg 15/25;
      initAngleY=0; minAngleY=-150; maxAngleY=+150; //Rg 100;
    };

    class ViewOptics : ViewOptics
    {
      initAngleX=0; minAngleX=0; maxAngleX=0;
      initAngleY=0; minAngleY=0; maxAngleY=0;
      initFov=0.5; minFov=0.5; maxFov=0.5;
    };

    extCameraPosition[]={0,1.4,-25};

    selectionRotorStill = "vrtule staticka";
    selectionRotorMove = "vrtule blur";

    memoryPointLRocket = "L raketa";
    memoryPointRRocket = "P raketa";

    memoryPointLDust = "levy prach";
    memoryPointRDust = "pravy prach";

    selectionFireAnim = "zasleh";

    leftDustEffect = "LDustEffects";
    rightDustEffect = "RDustEffects";

    // hovering dust effect (used for VTOL only)
    dustEffect = "HeliDust";
    waterEffect = "HeliWater";

    class Reflectors
    {
      class Reflector
      {
        color[] = {0.9, 0.8, 0.8, 1.0};
        ambient[] = {0.1, 0.1, 0.1, 1.0};
        position = "L svetlo";
        direction = "konec L svetla";
        hitpoint = "L svetlo";
        selection = "L svetlo";
        size = 0.5;
        brightness = 2;
      };
    };

    class MFD
    {
      class HUD: AirplaneHUD
      {
        /// adjust position inside of memory points lod
        borderLeft = 0.09;
        borderRight = 0.02;
        borderTop = 0.02;
        borderBottom = 0.1;
      };
    }; // list of MFDs

    class GunFire : WeaponFireGun
    {
    };
    class GunClouds : WeaponCloudsGun
    {
    };
    class MGunFire : WeaponFireMGun
    {
    };
    class MGunClouds : WeaponCloudsMGun
    {
    };

    class Sounds: Sounds
    {
      class Engine: Engine
      {
        sound = "soundEngine";
        frequency = "(1-randomizer*0.05)*(thrust+0.5)*rpm";
        volume = "thrust+0.5";
      };
      class Movement: Movement
      {
        sound = "soundEnviron";
        frequency = "1";
        volume = "speed*0.03334";
      };
    };
  };
  class Ship: AllVehicles
  {
    //--
    unitInfoType=RscUnitInfo;
    vehicleClass="Ship";

    icon = iconShip;

    cost=10000000;
    armor=1000;
    displayName=$STR_DN_SHIP;
    nameSound="veh_ship";
    accuracy=0.005;

    maxSpeed=30;

    simulation=ship;

    audible=6;
    fuelCapacity=100;

    formationX=50;
    formationZ=100;
    precision=25;
    brakeDistance=50; // vehicle movement precision
    formationTime=20;

    steerAheadSimul=2.0; // steer point on path properties
    steerAheadPlan=2.4;

    predictTurnSimul=2.0; // brake before turns
    predictTurnPlan=2.4;

    type=VArmor;
    //threat[] VSoft, VArmor, VAir
    threat[]={0.2, 0.2, 0.1};

    getInRadius = 10;

    driverAction = "";
    cargoAction[] = {""};

    getInAction = "";  // no animation
    getOutAction = ""; // no animation

    pointPilot = "pilot"; // searching in VIEW_PILOT
    pointCommander = "velitel"; // searching in VIEW_PILOT

    selectionFireAnim = "zasleh";

    selectionBrakeLights = "brzdove svetlo";

    memoryPointMissile[] = {"spice rakety", "usti hlavne"};
    memoryPointMissileDir[] = {"konec rakety", "konec hlavne"};

    /// ship can always float - what it be otherwise good for?
    /** not really used, but required by the common infrastructure */
    canFloat = true;
    soundGear[] = {"", db-75, 1};

    occludeSoundsWhenIn = db-5;
    obstructSoundsWhenIn = db-10;

    class Exhausts
    {
      class Exhaust1
      {
        position = "vyfuk start";
        direction = "vyfuk konec";
        effect = "ExhaustsEffect";
      };
    };
    leftDustEffect = "LDustEffects";
    rightDustEffect = "RDustEffects";

    leftWaterEffect = "LWaterEffects";
    rightWaterEffect = "RWaterEffects";

    leftEngineEffect = "LEngEffects";
    rightEngineEffect = "REngEffects";

	//memory points for rightEngineEffect and leftEngineEffect
    memoryPointsLeftEngineEffect = "EngineEffectL";
    memoryPointsRightEngineEffect = "EngineEffectR";
    class HitPoints
    {
      class HitEngine {armor=1.2;material=60;name="engine";visual="engine";passThrough=1;}
    };
    // new style of turrets definition - structure description
    class Turrets
    {
      class MainTurret : NewTurret
      {
        commanding = -1;

        startEngine = 0;
      };
    };

    class ViewPilot: ViewPilot
    {
      initFov=1.0; minFov=0.42; maxFov=1.2;
      initAngleX=6; minAngleX=-85; maxAngleX=+85; //Rg 8/10;
      initAngleY=0; minAngleY=-150; maxAngleY=+150; //Rg 100;
    };
    class ViewOptics : ViewOptics
    {
      initFov=0.7; minFov=0.07; maxFov=0.35;
    };
    class MarkerLights
    {
      class RedStill
      {
        name = "cerveny pozicni";
        color[] = {0.3, 0.03, 0.03, 1};
        ambient[] = {0.03, 0.003, 0.003, 1};
        brightness = 0.01;
        blinking = false;
      };
      class GreenStill
      {
        name = "zeleny pozicni";
        color[] = {0.03, 0.3, 0.03, 1};
        ambient[] = {0.003, 0.03, 0.003, 1};
        brightness = 0.01;
        blinking = false;
      };
      class WhiteStill
      {
        name = "bily pozicni";
        color[] = {0.3, 0.3, 0.3, 1};
        ambient[] = {0.03, 0.03, 0.03, 1};
        brightness = 0.01;
        blinking = false;
      };
    };

    class Sounds: Sounds
    {
      class Engine: Engine
      {
        sound = "soundEngine";
        frequency = "(1-randomizer*0.05)*1.2*(thrust*0.7+0.5)";
        volume = "(thrust*0.5)+0.5";
      };
      class Movement: Movement
      {
        sound = "soundEnviron";
        frequency = "(speed*0.03334)*1.3+(1-(speed*0.03334))*0.7";
        volume = "(speed*0.03334)+(1-(speed*0.03334))*0.1";
      };
    };
  };

  class SmallShip: Ship
  {
    //--
    cost=50000;
    armor=300;
    displayName=$STR_DN_SMALL_SHIP;
    accuracy=0.20;

    fuelCapacity=700;

    weapons[]={FakeWeapon};
    magazines[]={FakeWeapon};

    maxSpeed = 60;

    // boats are quite able to maneuver precisely and brake fast
    precision=6;
    brakeDistance=20; // vehicle movement precision

    class Reflectors
    {
      class Reflector
      {
        color[] = {0.9, 0.8, 0.8, 1.0};
        ambient[] = {0.1, 0.1, 0.1, 1.0};
        position = "L svetlo";
        direction = "konec L svetla";
        hitpoint = "L svetlo";
        selection = "L svetlo";
        size = 0.5;
        brightness = 2;
      };
    };
  };

  class BigShip: Ship
  {
    //--
    cost=10000000;
    displayName=$STR_DN_BIG_SHIP;
    accuracy=0.20;
    armor=1000;

    weapons[]= {FakeWeapon};
    magazines[]={FakeWeapon};
  };


  class Truck: Car
  {
    //--
    nameSound="veh_truck";

    formationX=20;
    formationZ=30;

    getInAction = "";
    getOutAction = "";

    class HitPoints : HitPoints
    {
      class HitLFWheel: HitLFWheel {armor=0.3;}
      class HitRFWheel: HitRFWheel {armor=0.3;}

      class HitLF2Wheel: HitLF2Wheel {armor=0.3;}
      class HitRF2Wheel: HitRF2Wheel {armor=0.3;}

      class HitLMWheel: HitLMWheel {armor=0.3;}
      class HitRMWheel: HitRMWheel {armor=0.3;}

      class HitLBWheel: HitLBWheel {armor=0.3;}
      class HitRBWheel: HitRBWheel {armor=0.3;}
    };

    damperSize = 0.2;
    outsideSoundFilter = true;

    // internal camera viewing limitations
    initFov=1.0; minFov=0.6; maxFov=1.4;

    /// truck turning takes longer, needs to be done soon enough
    steerAheadSimul=0.7;
    steerAheadPlan=0.6;
  };


  /*!
  \patch 1.86 Date 9/17/2002 by Ondra
  - Fixed: Parachutes no longer smoke when destroyed.
  */

  class ParachuteBase: Helicopter
  {
    scope = private;
    destrType=DestructNo;

    simulation=parachute;

    weaponSlots = WeaponSlotPrimary + WeaponSlotSecondary + 10*WeaponSlotItem + 2*WeaponSlotBinocular + WeaponSlotHandGun + 4*WeaponSlotHandGunItem;

    autocenter = false; // no autocenter - needed for RT animation

    vehicleClass="Air";
    displayName=$STR_DN_PARACHUTE;
    nameSound="veh_parachute";
    model="para";
    picture = pictureParachute;
    icon = iconParachute;
    accuracy=0.1;

    fuelCapacity=0;
    unitInfoType=RscUnitInfoSoldier;
    hideUnitInfo=true;
    irTarget = false;

    driverCanSee = CanSeeEye+CanSeeEar+CanSeePeripheral;
    driverAction = "";
    armor = 10;

    memoryPointPilot = "pilot"; // also searching in VIEW_PILOT
    animationOpen = "";
    animationDrop = "";

    class ViewOptics : ViewOptics
    {
      initAngleX=0; minAngleX=-40; maxAngleX=17;
      initAngleY=0; minAngleY=-100; maxAngleY=100;
      initFov=0.5; minFov=0.3; maxFov=1.2;
    };

    type=VSoft;
    //threat[] VSoft, VArmor, VAir
    threat[]={0.0, 0.0, 0.0};

    cost=100000;
    //side=TWest;
    weapons[]={};
    magazines[]={};

    soundGetIn[] = {"", db-70, 1};
    soundGetOut[] = {"", db-70, 1};
  };


  class LaserTarget: All
  {
    //-- laser designator dot
    reversed=false; // all vehicles are reversed - except static

    hasDriver=false;

    icon = iconLaserTarget;

    displayName = $STR_DN_LASER_TARGET;
    nameSound="obj_LaserTarget";
    // laser target can hardly be non recognized
    accuracy=0.0001;
    model = "laserTgt.p3d";

    picture = pictureLaserTarget;
    /*!
    \patch 1.33 Date 11/29/2001 by Ondra
    - Changed: Laser target priority increased.
    */
    // add weapons - make AI consider it "combat" enemy
    // but not too dangerous, otherwise it will try to avoid it
    weapons[]={FakeWeapon};
    magazines[]={FakeWeapon};

    type=VArmor;
    //threat[] VSoft, VArmor, VAir
    threat[]={0.5, 0.5, 0.1};
    maxSpeed=0; // cannot move

    simulation="laserTarget";
    laserTarget = true;
    irTarget = false;

    destrType=DestructNo;
    side = TCivilian;

    cost=1e15; // make AI attacking it with very high priority
    armor=500; // make AI selecting appropriate (heavy) weapon
  };
  class NVTarget: All
  {
    //-- laser designator dot
    reversed=false; // all vehicles are reversed - except static

    hasDriver=false;

    icon = iconLaserTarget;

    displayName = $STR_DN_LASER_TARGET;
    nameSound="obj_LaserTarget";
    // laser target can hardly be non recognized
    accuracy=0.0001;
    model = "laserTgt.p3d";

    picture = pictureLaserTarget;
    /*!
    \patch 1.33 Date 11/29/2001 by Ondra
    - Changed: Laser target priority increased.
    */
    // add weapons - make AI consider it "combat" enemy
    // but not too dangerous, otherwise it will try to avoid it
    weapons[]={FakeWeapon};
    magazines[]={FakeWeapon};

    type=VArmor;
    //threat[] VSoft, VArmor, VAir
    threat[]={0.5, 0.5, 0.1};
    maxSpeed=0; // cannot move

    simulation="nvmarker";
    laserTarget = false;
    irTarget = false;
    nvTarget = true;

    destrType = DestructEngine;
    side = TCivilian;

    cost=1e15; // make AI attacking it with very high priority
    armor=500; // make AI selecting appropriate (heavy) weapon
  };

  class ArtilleryTarget: All
  {
    //-- laser designator dot
    reversed=false; // all vehicles are reversed - except static

    hasDriver=false;

    icon = iconLaserTarget;

    displayName = "ArtilleryTarget";
    nameSound="";
    // artillery target can hardly be non recognized
    accuracy=0.0001;
    model = "";

    picture = pictureLaserTarget;

    // add weapons - make AI consider it "combat" enemy
    // but not too dangerous, otherwise it will try to avoid it
    weapons[]={FakeWeapon};
    magazines[]={FakeWeapon};

    type=VArmor;
    //threat[] VSoft, VArmor, VAir
    threat[]={0.5, 0.5, 0.1};
    maxSpeed=0; // cannot move

    simulation="artillerymarker";
    laserTarget = false;
    irTarget = false;
    nvTarget = false;
    artilleryTarget = true;

    destrType = DestructNo;
    side = TCivilian;

    cost=1e15; // make AI attacking it with very high priority
    armor=500; // make AI selecting appropriate (heavy) weapon

    scope = protected;
  };

  class ArtilleryTargetW: ArtilleryTarget
  {
    side = TWest;
    scope = protected;
  }
  class ArtilleryTargetE: ArtilleryTarget
  {
    side = TEast;
    scope = protected;
  }

  //Special vehicle used in pathfinding.
  class PaperCar: Car
  {
    scope = protected;
    side = TCivilian;
    picture = picturePaperCar;
    icon = iconPaperCar;
    displayName = $STR_DN_PAPER_CAR;
    model = "\core\default\default.p3d";
    class Reflectors {};
  };

  //Special vehicle used for fire sector scanning of AI.
  class FireSectorTarget: All
  {
    scope = protected;
    side = TCivilian;
    picture = "";
    icon = "";
    nameSound = "";
    //ToDo: Stringtablize?
    displayName = "Internal: Fire Sector Target";
    vehicleClass = "Objects";

    model = "\core\default\default.p3d";

    weapons[] = {};
    magazines[] = {};
    transportAmmo = 0;
    transportRepair = 0;
    transportFuel = 0;
    supplyRadius = 0;

    type = VSoft;
    threat[] = {0, 0, 0};
    maxSpeed = 0;
    armor = 100;

    simulation = "house";
    accuracy = 0.005;
    cost = 100;
    destrType = 0;
    mapSize = 0.1;
    reversed = false;
    hasDriver = false;
    coefInside = 0;
    coefInsideHeur = 0;
    windSockExist = 0;
    animated = false;
    ladders[] = {};
    typicalCargo[] = {};
  };


  // STATIC BUILDINGS
  class Static: All
  {
    //-- static object or building
    reversed=false; // all vehicles are reversed - except static

    hasDriver=false;

    icon = iconStaticObject;

    displayName=$STR_DN_UNKNOWN;
    nameSound="obj_object";
    accuracy=0.005;
    simulation="house";

    cost=1000; // default building is assumed to be very cheap

    picture = pictureStaticObject; // TODO: use actual building pictures instead
    weapons[]={};
    magazines[]={};

    irTarget = false;
    targetCategory="building";

    type=VArmor;
    //threat[] VSoft, VArmor, VAir
    threat[]={0.0, 0.0, 0.0};
    maxSpeed=0; // cannot move
    /*
    coefInside = 6; //cost of way inside building for AI
    coefInsideHeur = 4.3; //cost of way inside building for AI
    */
    coefInside = 1; // cost of way inside building for AI (multiplier for minCost)
    coefInsideHeur = 4; // no more used

    windSockExist = 0;
  };

  class Fortress: Static
  {
    displayName=$STR_DN_BUNKER;
    nameSound="obj_Bunker";
    armor=1000;
    cost=1000000;

    accuracy=0.20;
  };

  class Building: Static
  {
    scope = protected; // cannot be private, buildings in landscape are created with this type
    displayName=$STR_DN_BUILDING;
    nameSound="obj_building";
    armor=150;
    accuracy=0.10;
  };


  class NonStrategic : Building
  {
    side=TCivilian;
    ladders[]={};

    class DestructionEffects
    {
      class Sound
      {
        simulation = "sound";
        type = "DestrHouse";
        position = "destructionEffect1";

        intensity = 1;
        interval = 1;
        lifeTime = 0.05;
      };
      class DestroyPhase1
      {
        simulation = "destroy";
        type = "DelayedDestruction";

        lifeTime = 2.5;
        // for destroy parameters do not matter
        position = "";
        intensity = 1;
        interval = 1;
      };
      class DamageAround1
      {
        simulation = "damageAround";
        type = "DamageAroundHouse";

        // for damage around parameters do not matter?
        position = "";
        intensity = 1;
        interval = 1;
        lifeTime = 1;
      };
    };
  };
  class Strategic : Building
  {
    side=TCivilian;
    armor=250;
    cost=1000000; /// strategic targets are considered very expensive
    ladders[]={};
  };

  class FlagCarrierCore: Strategic
  {
    scope = protected;
    animated = false; // flagpole itself is not animated, only flag proxy is
    vehicleClass = "Objects";
    icon = "";
    displayName = $STR_DN_FLAG;
    simulation = "flagcarrier";
    model = "";
    placement = vertical;
    mapSize = 5;
    animationFlag = "";
  };

  class Land_VASICore: NonStrategic
  {
    scope = protected;

    animated = false;
    reversed = false; // do not reverse to avoid Buldozer/game confusion

    vehicleClass = "Objects";
    icon = "";
    model = "";
    displayName = "VASI";
    accuracy = 0.20;
    typicalCargo[] = {};
    destrType = DestructBuilding;

    selectionWhiteLight = "light-white";
    selectionRedLight = "light-red";
    selectionOffLight = "light-off";

    irTarget = false;

    transportAmmo = 0;
    transportRepair = 0;
    transportFuel = 0;

    cost=0;
    armor=100;
    mapSize = 6.4;

    simulation = "vasi";

		enabled = false;
    //@{ vasi specific parameters
    /// color below glide path
    vasiRed[]={1,0,0};
    /// color above glide path
    vasiWhite[]={1,1,1};
    /// glide path angle (ILS percentage, same as in ilsDirection)
    vasiSlope=0.08;
    //@}
  };

  // THINGS
  /*
  class Thing: Strategic
  {
  };
  */
  /**/
  class Thing: All
  {
    reversed=false; // all vehicles are reversed - except static
    animated=false;

    icon = iconThing;
    vehicleClass="Objects";

    displayName=$STR_DN_UNKNOWN;
    nameSound="obj_object";
    accuracy=0.005;
    simulation="thing";
    side=TCivilian;

    weight = 0;
    class InventoryPlacements {};

    submerged = 0; // normal things have no submersion
    submergeSpeed = 0;

    /// air friction depending on velocity 2, 1 and 0 powers
    airFriction2[]={0.01,0.01,0.01};
    airFriction1[]={0.01,0.01,0.01};
    airFriction0[]={0.01,0.01,0.01};

    /// how much air friction can cause a rotation
    airRotation=0;
    // the simulation is unable to handle very high air friction
    // we simulate floating by reducing gravity instead
    gravityFactor = 1;

    timeToLive = 1e10; // infinite time to live
    disappearAtContact = false;
    hasDriver=false;

    picture = pictureThing; // TODO: use actual building pictures instead
    weapons[]={};
    magazines[]={};

    minHeight = 0.1;
    avgHeight = 0.2;
    maxHeight = 0.4;

    type=VArmor;
    //threat[] VSoft, VArmor, VAir
    threat[]={0.0, 0.0, 0.0};
    maxSpeed=0; // cannot move
    irTarget = false;
  };

  class ThingEffect: Thing
  {
    scope=protected; // protected when finished

    simulation="thingeffect";
    irTarget = false;

    submerged = -0.5; // initial submersion, <0 initial delay
    submergeSpeed = 0.25;
    timeToLive = 20; // always disappear after certain period of time
  };

  /// very light objects - leaves
  class ThingEffectLight: ThingEffect
  {
    scope = private;
    // air friction is very high in Z-axis for things like this
    airFriction2[]={1,1,8};
    airFriction1[]={1,1,4};
    airFriction0[]={0.1,0.1,0.1};

    // we can rotate a lot
    airRotation=0.1;

    submergeSpeed = 0;

    //@{ ambient "life" creation parameters
    minHeight = 0.1;
    avgHeight = 0.2;
    maxHeight = 0.4;
    //@}
  };
  /// very light objects - floating feathers
  class ThingEffectFeather: ThingEffectLight
  {
    //More symmetrical friction than other clutter, meaning the pollen moves only very slightly in all directions.
    airFriction2[] = {16, 16, 16};
    airFriction1[] = {16, 16, 16};
    airFriction0[] = {0.1, 0.1, 0.1};

    // the simulation is unable to handle very high air friction
    // we simulate floating by reducing gravity instead
    gravityFactor = 0.05;
    // can float quite high
    minHeight = 0.5;
    avgHeight = 1.8;
    maxHeight = 3;
  };

  class FxExploArmor1: ThingEffect
  {
    access = ReadAndWrite;
    model = "";
    displayName="Internal: FxExploArmor1";
  };
  class FxExploArmor2: ThingEffect
  {
    access = ReadAndWrite;
    model = "";
    displayName="Internal: FxExploArmor2";
  };
  class FxExploArmor3: ThingEffect
  {
    access = ReadAndWrite;
    model = "";
    displayName="Internal: FxExploArmor3";
  };
  class FxExploArmor4: ThingEffect
  {
    access = ReadAndWrite;
    model = "";
    displayName="Internal: FxExploArmor4";
  };
  class FxCartridge: ThingEffect
  {
    access = ReadAndWrite;
    model = "";
    displayName=$STR_DN_FX_CARTRIDGE;

    submerged = -0; // initial submersion, <0 inital delaty
    submergeSpeed = 0;
    timeToLive = 5; // always disappear after certain period of time

    disappearAtContact=true;
  };
  /**/
};

class CfgNonAIVehicles
{
  access = ReadAndWrite;

  class EmptyDetector
  {
    scope = public;
    displayName = $STR_DN_EMPTY;
    simulation = "detector";

    //Changed from empty.p3d by Joris.
    model = "";
    selectionFabric = "latka";
  };

  class StreetLamp
  {
    model = "";
    destrType = DestructTree;
    simulation = StreetLamp;
    animated = false;

    colorDiffuse[] = {0.9,0.8,0.6};
    colorAmbient[] = {0.1,0.1,0.1};
    brightness = 0.2; // 1 == light is full at 50 m

    // note: armor is different from AI entities
    // it is direct number, not relative to overall armor
    class HitPoints
    {
      class HitBulb {armor=1;material=60;name="lampa";passThrough=1;}
    };

    armorStructural = 1;

    class Reflectors
    {
      class LampLight
      {
        color[] = {0.9, 0.8, 0.6, 1.0};
        ambient[] = {0.1, 0.1, 0.1, 1.0};
        position = "Light";
        direction = "";
        hitpoint = "lampa";
        selection = "";
        size = 0.5;
        brightness = 0.2;
      };
    };
    aggregateReflectors[] = {};
    armorLights=1;
  };

  class EditCursor
  {
    model = "\core\cursor\cursor.p3d";
    simulation = EditCursor;
  };

  class ObjView
  {
    scope = public;
    model = "";
    simulation = ObjView;
  };

  class Temp
  {
    scope = public;
    model = "";
    simulation = Temp;
  };

  class Bird
  {
    scope = private;
    model="";
    animated=false;
    simulation = SeaGull;
    reversed = false;
    minHeight=5;
    avgHeight=10;
    maxHeight=50;
    minSpeed=-0.5; // m/s
    maxSpeed=20; // m/s
    acceleration = 7; //m/s^2
    turning = 1; // angular acceleration - relative (the higher, the more maneuvrable)
    straightDistance=50;
    flySound[]={"",db-30,1, 1};
    singSound[]={"",db-30,1, 1};
    canBeShot=true;
    airFriction2[]={25,12,2.5};
    airFriction1[]={1500,700,100};
    airFriction0[]={40,20,60};
  };
  /*!
  \patch_internal 5156 Date 5/11/2007 by Ondra
  - Optimized: Some insects were using dynamic vertex buffers. (Real performance hit next to none)
  */
  class Insect: Bird
  {
    animated=false; // no insects we have use any animation, only birds
    minHeight=-0.10; // allow landing
    avgHeight=1.2;
    maxHeight=2.0;
    minSpeed=-0.1; // m/s
    maxSpeed=5; // m/s
    acceleration = 25; //m/s^2
    straightDistance=2;
    turning = 2;
    flySound[]={"",db-30,1, 1};
    singSound[]={"",db-30,1, 1};
    canBeShot=false;
    airFriction2[]={125,12,2.5};
    airFriction1[]={7500,700,100};
    airFriction0[]={200,20,60};
  };
  class SeaGull: Bird
  {
    scope = public;
    //Changed from racekT.p3d by Joris.
    model = "\core\default\default.p3d";
    reversed = false;
  };
  class Camera
  {
    scope = public;
    model = "";
    simulation = Camera;
    //Changed from krizek.paa by Joris.
    crossHairs = "#(argb,8,8,3)color(1,1,1,1)";
  };

  class CamConstruct
  {
    scope = public;
    model = "";
    simulation = CamConstruct;
    //construction radius
    radius = 125;
    //max. height above land
    maxHAL = 30;
    crossHairs = "#(argb,8,8,3)color(1,1,1,1)";
  };

  // proxy object classes - new inventory system
  // item in hands
  class ProxyHands
  {
    scope = public;
    simulation = ProxyInventory;
    inventorySlot = Hands;
    autocenter = false; // normal vehicle are autocentered
    animated = false;
    model = "";
    shadow = true;
    reversed = true;
  };
  // items in slots
  class ProxyEyewear
  {
    scope = public;
    simulation = ProxyInventory;
    inventorySlot = Eyewear;
    autocenter = false; // normal vehicle are autocentered
    animated = false;
    model = "";
    shadow = true;
    reversed = true;
  };
  class ProxyHeadgear
  {
    scope = public;
    simulation = ProxyInventory;
    inventorySlot = Headgear;
    autocenter = false; // normal vehicle are autocentered
    animated = false;
    model = "";
    shadow = true;
    reversed = true;
  };
  class ProxyBody
  {
    scope = public;
    simulation = ProxyInventory;
    inventorySlot = Body;
    autocenter = false; // normal vehicle are autocentered
    animated = false;
    model = "";
    shadow = true;
    reversed = true;
  };
  class ProxyLegs
  {
    scope = public;
    simulation = ProxyInventory;
    inventorySlot = Legs;
    autocenter = false; // normal vehicle are autocentered
    animated = false;
    model = "";
    shadow = true;
    reversed = true;
  };
  class ProxyFeet
  {
    scope = public;
    simulation = ProxyInventory;
    inventorySlot = Feet;
    autocenter = false; // normal vehicle are autocentered
    animated = false;
    model = "";
    shadow = true;
    reversed = true;
  };
  class ProxyVest
  {
    scope = public;
    simulation = ProxyInventory;
    inventorySlot = Vest;
    autocenter = false; // normal vehicle are autocentered
    animated = false;
    model = "";
    shadow = true;
    reversed = true;
  };
  class ProxyGloves
  {
    scope = public;
    simulation = ProxyInventory;
    inventorySlot = Gloves;
    autocenter = false; // normal vehicle are autocentered
    animated = false;
    model = "";
    shadow = true;
    reversed = true;
  };
  class ProxyBack
  {
    scope = public;
    simulation = ProxyInventory;
    inventorySlot = Back;
    autocenter = false; // normal vehicle are autocentered
    animated = false;
    model = "";
    shadow = true;
    reversed = true;
  };
  class ProxyShoulder
  {
    scope = public;
    simulation = ProxyInventory;
    inventorySlot = Shoulder;
    autocenter = false; // normal vehicle are autocentered
    animated = false;
    model = "";
    shadow = true;
    reversed = true;
  };
  class ProxyMelee
  {
    scope = public;
    simulation = ProxyInventory;
    inventorySlot = Melee;
    autocenter = false; // normal vehicle are autocentered
    animated = false;
    model = "";
    shadow = true;
    reversed = true;
  };
  class ProxyMask
  {
    scope = public;
    simulation = ProxyInventory;
    inventorySlot = Mask;
    autocenter = false; // normal vehicle are autocentered
    animated = false;
    model = "";
    shadow = true;
    reversed = true;
  };

  // common attachment
  class ProxyAttachment
  {
    scope = private;
    simulation = ProxyInventory;
    inventorySlot = ;
    autocenter = false; // normal vehicle are autocentered
    animated = false;
    model = "";
    shadow = true;
    reversed = true;
  };

  class Proxyus_bag : ProxyBack
  {
  };

  class ProxyHide
  {
    autocenter = false; // normal vehicle are autocentered
    scope = public;
    animated = false;
    model = "";
    simulation = "alwayshide";
  };

  class ProxyWreck
  {
    autocenter = false; // proxy never autocentered
    scope = private;
    animated = false;
    model = "";
    simulation = "alwaysshow";
  };

  //Crew proxies
  class ProxyCrew
  {
    autocenter = false; // proxy never autocentered
    scope = protected;
    model = "";
    shadow = true;
    simulation = proxycrew;
    crewPosition = CPDriver;
  };

  class ProxyCommander: ProxyCrew {crewPosition = CPCommander;};
  class ProxyDriver: ProxyCrew {crewPosition = CPDriver;};
  class ProxyGunner: ProxyCrew {crewPosition = CPGunner;};
  class ProxyCargo: ProxyCrew {crewPosition = CPCargo;};

  class ProxyFlag
  {
    autocenter = false; // normal vehicle are autocentered
    scope = public;
    reversed = false;
    model = "";
    shadow = true;
    simulation = "flag";
    selectionFabric = "latka";
  };

  class RandomShape
  {
    models[] = {};
    autocenter = false; // normal vehicle are autocentered
    scope = public;
    reversed = false;
    shadow = true;
    model = "";
    simulation = "randomshape";
  };

  class ProxyAmmoInTruck
  {
    autocenter = false; // normal vehicle are autocentered
    scope = public;
    reversed = false;
    animated = false;
    shadow = true;
    model = "";
    simulation = "alwaysshow";
  };
};

class CfgSimulationCosts
{
  tank = 150;
  zsu = 150;
  car = 120;
  motorcycle = 120;
  ship = 140;
  soldierold = 100;
  soldier = 100;
  helicopter = 300;
  parachute = 120;
  airplane = 200;
  lasertarget = 100;
  nvmarker = 100;
  house = 10;
  thing = 10;
  thingeffect = 10;
  church = 100;
  fire = 50;
  fountain = 100;
  seagull = 10;
  camera = 10;
  flagcarrier = 50;
  invisible = 10;
  mines = 10;
  sounds = 10;
};

class CfgMarkedTargets
{
  class Inf
  {
    type = "man";
    importance = 1;    //importances should differ
    battleRadius = 50; //meters
    farRadius = 150;   //what is over 150 meters is far
  };
  class MotorInf
  {
    type = "car";
    importance = 3;
    battleRadius = 100;
    farRadius = 400;
  };
  class MechInf
  {
    type = "tank";
    transportSoldiers = 1; //to distinguish between tank and APC
    importance = 4;
    battleRadius = 100;
    farRadius = 400;
  };
  class Armored
  {
    type = "tank";
    importance = 5;
    battleRadius = 100;
    farRadius = 400;
  };
  class Air
  {
    type = "helicopter";
    importance = 7;
    battleRadius = 400;
    farRadius = 800;
  };
  class Plane
  {
    type = "plane";
    importance = 8;
    battleRadius = 400;
    farRadius = 800;
  };
  class UAV
  {
    type = "uav";
    importance = 9;
    battleRadius = 100;
    farRadius = 300;
  };
  class Art
  {
    type = "staticcanon";
    importance = 10;
    battleRadius = 400;
    farRadius = 800;
  };
  class Mortar
  {
    type = "staticmortar";
    importance = 11;
    battleRadius = 400;
    farRadius = 800;
  };
};

/// ReportTargetsChangeFactors serve to determine, how important change is neccessary in order to be reported.
/// Each item corresponds to 1 second delay. if (targetGrpWeight/targetGrpOLDWeight>factor) DoReport();
/// Example: after three seconds, the factor is ReportTargetsChangeFactors[2]==2.0
ReportTargetsChangeFactors[] = {3.0, 2.5, 2.0, 1.6, 1.3};
/// Inititial cost is used when target group change is to be compared to the empty group
/// Example: 1 man = cost 1 will be reported after the factor decrease under 1/ReportTargetsEmptyGroupCost=1/0.4=2.5
//           and as ReportTargetsChangeFactor[1]==2.5, it will be reported after TWO seconds aggregation delay
//           (more men can be detected meanwhile - and aggregated into one radio report)
ReportTargetsEmptyGroupCost = 0.4;

class CfgFSMs
{
  access = ReadAndCreate;
};

class PreloadVehicles
{
  access = ReadAndCreate;
};

// empty container for additional skeleton informations
class CfgSkeletonParameters{}; 

class CfgRecipes
{
};

class CfgObjectActions
{
};

class CfgSurfaceActions
{
};