#ifdef _MSC_VER
#pragma once
#endif

#ifndef _cutTypes_hpp_
#define _cutTypes_hpp_

// universal conversions

//inline int toInt( int x ) {return int(x);}

//typedef word Pixel;
//typedef DWORD Pixel32;
//typedef DWORD PixelGen;

#define MAX_Z 1.0f

// list of all classes defined
// file objects
class QIStream;
class QOStream;
class QIFStream;
class QOFStream;
//class LoadStream;
//class SaveStream;

class SecondaryThread;

// object
class Visual;
class Frame;
class Shape;
class LODShape;
class LODShapeWithShadow;
class Object;
class Camera;

// animations
class AnimationPhase;
class Animation;
class AnimationWithCenter;

// object clases
class Church;
class DogHouse;
class ChickenHouse;
class BirdsTree;

// vehicles

class Simul;
class EntityAI;
class Entity;
typedef EntityAI VehicleWithAI;
typedef Entity Vehicle;
class Person;

class EntityAIType;
class EntityType;
typedef EntityAIType VehicleType;
typedef EntityType VehicleNonAIType;

class AttachedOnVehicle;
class Transport;

class PauseMenu;
class AbstractUI;
class AbstractOptionsUI;

class AmmoType;
class WeaponInfo;

class Tank;
class TankType;
class TankWithAI;
class Helicopter;
class HelicopterAuto;
class HelicopterType;

class SeaGull;
class SeaGullAuto;

//class ZSU;
class Car;
class CarType;

class Man;
class ManType;
#define SoldierWithAI Soldier // TODO: Remove SoldierWithAI class
class Soldier;

class Shot;
class Missile;
class ShotShell;
class ShotBullet;
class Smoke;
class Explosion;
class Cloudlet;
class SmokeSourceVehicle;
class SmokeSourceOnVehicle;

class CloudletSource;
class SmokeSource;
class DustSource;
class WeaponCloudsSource;

// AI

class AI;
class AIUnit;
class AISubgroup;
class AIGroup;
class AICenter;

// Mission Templates
struct ArcadeTemplate;
struct ArcadeIntel;
struct ArcadeGroupInfo;
struct ArcadeUnitInfo;
struct ArcadeWaypointInfo;
struct ArcadeSensorInfo;
struct ArcadeFlagInfo;
struct ArcadeMarkerInfo;
struct ArcadeBuildingInfo;

class TankPilot;
class HeliPilot;
class CarPilot;
class SoldierPilot;

class Animator;
class AnimatorLinear;
class AnimatorSet;

// scene
class Scene;
class World;
class Landscape;
class WaterLevel;

union GeographyInfo;

// timing
class AbstractTime;
class Time;
class TimeSec;
class UITime;

// math3d

//class Matrix4;
//class Matrix3;
//class Vector4;
//class Vector3;

// vertex
class Color;
class Vertex;
class TLVertex;
class VertexTable;
class TLVertexTable;
//class VertexMesh;
//class TLVertexMesh;

// different engine implementations

class Engine;

class EngineMyst;
class EngineGlide;

class EngineDD;
class EngineRampFull;
class EngineRampWin;

class EngineD3DFull;
class EngineD3DWin;

// polygons
class Poly;

class SFaceArray;
class FaceArray;

class ClippedFaces;

// material
//class Material;

// light
class LightDirectional; // main light types
class LightSun;

class Light; // additional light types
class LightPoint;
class LightReflector;

class LightList;

// textures for software ramp mode
class Ramp;
class TextureRamp;
class TextBankRamp;
class MipmapLevelRamp;

// textures for Glide
class RampGlide;
class TextureGlide;
class TextBankGlide;
class MipmapLevelGlide;

// textures for D3D
class RampD3D;
class TextureD3D;
class TextBankD3D;
class MipmapLevelD3D;

// abstract textures

class PacLevel;
class Texture;
class AbstractTextBank;
class AnimatedTexture;

class Font;

#define AbstractMipmapLevel PacLevelMem
class PacLevelMem;
class PacLevel;

// 3D sound engine

class AbstractWave;
class AbstractSoundSystem;

class Wave;
class SoundSystem;

class RadioChannel;
class RadioMessage;
class Speaker;

// enums
//typedef int VertexIndex;
typedef short VertexIndex; // better memory usage


//typedef int LightHandle;

typedef unsigned int ClipFlags;
enum
{
	ClipNone=0,
	ClipFront=1,ClipBack=2,
	ClipLeft=4,ClipRight=8,
	ClipBottom=16,ClipTop=32,
	ClipUser0=0x40,
	//ClipUser1=0x80, // currently not used
	ClipAll = ClipFront|ClipBack|ClipLeft|ClipRight|ClipBottom|ClipTop,

	// surface attributes
	ClipLandMask=0xf00,ClipLandStep=0x100,
	ClipLandNone=ClipLandStep*0, // exclusive - "or" and "and" used
	ClipLandOn=ClipLandStep*1,
	ClipLandUnder=ClipLandStep*2,
	ClipLandAbove=ClipLandStep*4,
	ClipLandKeep=ClipLandStep*8,

	// decal attributes
	ClipDecalMask=0x3000,ClipDecalStep=0x1000,
	ClipDecalNone=ClipDecalStep*0, // exclusive - "or" and "and" used
	ClipDecalNormal=ClipDecalStep*1,
	ClipDecalVertical=ClipDecalStep*2,

	// fogging attributes
	ClipFogMask=0xc000,ClipFogStep=0x4000,
	ClipFogNormal=ClipFogStep*0,
	ClipFogDisable=ClipFogStep*1,
	ClipFogSky=ClipFogStep*2,
	ClipFogShadow=ClipFogStep*3,

	// per vertex lighting attributes
	ClipLightMask=0xf0000,ClipLightStep=0x10000,
	ClipLightNormal=ClipLightStep*0,
	ClipLightSky=ClipLightStep*1,
	ClipLightCloud=ClipLightStep*2,
	ClipLightSun=ClipLightStep*3,
	ClipLightSunHalo=ClipLightStep*4,
	ClipLightMoon=ClipLightStep*5,
	ClipLightMoonHalo=ClipLightStep*6,
	ClipLightStars=ClipLightStep*7,
	ClipLightLine=ClipLightStep*8, // line - alpha used to simulate width

	ClipUserMask=0xff00000,ClipUserStep=0x100000, // used for star brigtness...
	MaxUserValue=0xff,
		
	//ClipVarMask=0xf0000000,ClipVarStep=0x10000000,
	//ClipUserIsPalette=ClipVarStep*8U,

	//ClipUserIsAlpha=ClipVarStep*1,
	//ClipUserIsBrightness=ClipVarStep*2,
	//ClipUserIsDammage=ClipVarStep*4,

	// all hints mask
	ClipHints=ClipLandMask|ClipDecalMask|ClipFogMask|ClipLightMask|ClipUserMask
};

enum
{
	GrassTexture=1, // special grass layer - use special detail texture
	OnSurface=2, //UnderSurface=4,AboveSurface=8,
	// Nonplanar means that polygon cannot be culled
	// OnSurface means that polygon should be splitted to fit landscape surface
	IsOnSurface=4,NoZBuf=8,NoZWrite=0x10,
	// NoZBuf,NoZWrite disable parts of z-buffer functionality
	NoShadow=0x20,IsShadow=0x40,ShadowDisabled=0x80,
	// NoShadow means that the face should not cast a shadow
	// IsShadow means that the polygon is shadow polygon (dark&transparent)
	// ShadowDisabled means face has been light culled and should not be drawn
	IsAlpha=0x100, // alpha transparent texture - should be z-sorted before drawing
	IsTransparent=0x200, // is chromakey texture (should be z-sorted before drawing)
	IsWater=0x400,IsLight=0x800,
	//IsFlare=0x1000,
	PointSampling=0x1000, // disable is bi- or tri- linear filter
	// IsWater means special transparent face used for reflections
	// IsLight is transparent face used for volumetrical lights
	// IsFlare is transparent face used for flares (basically the same as IsLight)
	NoClamp=0x2000,ClampU=0x4000,ClampV=0x8000,
	// NoClamp means that the texture is tiling and need not be clamped
	IsAnimated=0x10000, // there is an animated texture attached on this face
	//DoAntiAliasing=0x20000,	// toggle AA on/off
	IsAlphaOrdered=0x20000,	// alpha order important - cannot reorder
	NoDropdown=0x40000, // disable speed drop down
	IsAlphaFog=0x80000, // use alpha blending instead of fog
	FogDisabled=0x100000, // fog disabled for entire object
	IsColored=0x200000, // object constant color is set
	IsHidden=0x400000, // face hidden
	BestMipmap=0x800000, // best mipmap forced
	DetailTexture=0x1000000, // use default detail texture
	SpecularTexture=0x2000000, // use default specular texture
	ZBiasMask=0xc000000,ZBiasStep=0x4000000, // control z-bias
	IsHiddenProxy=0x10000000, // face hard-hidden (proxy)
	//PointFilter=0x10000000,
	NoTexMerger=0x20000000, // disable texture merging
	SpecLighting=0x40000000, // lighting unsuitable for D3D
	DisableSun=0x80000000,
};


#endif

