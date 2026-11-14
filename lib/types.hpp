#ifdef _MSC_VER
#pragma once
#endif

#ifndef _TYPES_HPP
#define _TYPES_HPP

// types should be removed - this type of forward declaration is not modular
// universal conversions

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

// object
class Visual;
class Frame;
class Shape;
class LODShape;
class LODShape;
class Object;
class Camera;

// animations
class AnimationPhase;
class Animation;

// vehicles

class Simul;
class EntityAI;
class Entity;
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
#if _HELISIM_ROTORLIB
class HelicopterRTD;
#endif
class HelicopterAuto;
class HelicopterType;

class SeaGull;
class SeaGullAuto;

class Car;
class CarType;

class Man;
class ManType;
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

// Mission Templates
struct IMission;
struct ArcadeTemplate;
struct ArcadeIntel;
struct ArcadeUnitInfo;
struct ArcadeFlagInfo;
struct ArcadeMarkerInfo;
struct ArcadeBuildingInfo;

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

// polygons
class Poly;

class SFaceArray;
class FaceArray;

class ClippedFaces;

// light
class LightDirectional; // main light types
class LightSun;

class Light; // additional light types
class LightPoint;
class LightReflector;

class LightList;

// abstract textures

class PacLevel;
class Texture;
class AbstractTextBank;
class AnimatedTexture;

class Font;

class PacLevelMem;
class PacLevel;

// 3D sound engine

class AbstractWave;
class AbstractSoundSystem;

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
	//ClipFogShadow=ClipFogStep*3,

	// per vertex lighting attributes
	ClipLightMask=0xf0000,ClipLightStep=0x10000,
	ClipLightNormal=ClipLightStep*0,
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
	SunPrecalculated=1, //sun is pre-calculated and should not be calculated
	OnSurface=2, //UnderSurface=4,AboveSurface=8,
	// OnSurface means that polygon should be split to fit landscape surface
	IsOnSurface=4,NoZBuf=8,NoZWrite=0x10,
	// NoZBuf,NoZWrite disable parts of z-buffer functionality
	// NoAlphaWrite disable stencil shadow/alpha writes
	NoShadow=0x20,IsShadow=0x40,NoAlphaWrite=0x80,
	// NoShadow means that the face should not cast a shadow
	// IsShadow means that the polygon is shadow polygon (dark&transparent)
	IsAlpha=0x100, // alpha transparent texture - should be z-sorted before drawing
	IsTransparent=0x200, // is chroma-key texture (will enable alpha testing)
  IsShadowVolume=0x400, // determine to render geometry as a shadow volume
	IsLight=0x800, 
	DstBlendOne=IsLight, // use one as a dest. blend
	ShadowVolumeFrontFaces=0x1000, // determine whether we draw front faces or back faces of the shadow volume
	NoBackfaceCull=ShadowVolumeFrontFaces, // value sharing - cannot be used at the same time
	// used to extract clampUV as a pair - ((spec>>ClampLog)&ClampMask)
	ClampLog = 14, ClampMask = 3,
	NoClamp=0x2000,ClampU=0x4000,ClampV=0x8000,
	// NoClamp means that the texture is tiling and need not be clamped
	IsAnimated=0x10000, // there is an animated texture attached on this face
	IsAlphaOrdered=0x20000,	// alpha order important - cannot reorder
	//NoDropdown=0x40000, // disable speed drop down
  NoColorWrite=0x40000, // Just alpha will be rendered
	IsAlphaFog=0x80000, // use alpha blending instead of fog
	DstBlendZero=0x100000, // alpha blending - destination zero
	IsColored=0x200000, // object constant color is set
	IsHidden=0x400000, // face hidden
	BestMipmap=0x800000, // best mipmap forced
	FilterMask =0x3000000, // mipmap filtering mask
	FilterTrilinear=0x0000000, // default - tri-linear filtering
	FilterLinear=0x1000000, // default - bi-linear filtering, nearest mip-map
	FilterAnizotrop=0x2000000, // anisotropic filtering
	FilterPoint=0x3000000, // point sampling (not used)
	ZBiasMask=0xc000000,ZBiasStep=0x4000000, // control z-bias
	IsHiddenProxy=0x10000000, // face hard-hidden (proxy)
	NoStencilWrite=0x20000000, // disable stencil write
	TracerLighting=0x40000000, // lighting of the tracer
	DisableSun=0x80000000,
};

#define UISHADOW IsHiddenProxy

#endif

