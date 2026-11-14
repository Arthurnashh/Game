/* Poseidon game configuration */
/* Part of game design, no user editable data here. */

#include "CommonDefs.hpp"

// some basic defines
#define TEast 0
#define TWest 1
#define TGuerrila 2
#define TCivilian 3
#define TSideUnknown 4
#define TEnemy 5
#define TFriendly 6
#define TLogic 7

/* defined in commonDefs
#define true 1
#define false 0
*/

// type scope
// used for show entry

#define private 0 //! item is never visible
#define protected 1 //! "advanced designer" switch must be activated to use it
#define public 2 //! anyone can see/use it

/*defined in commonDefs
#define ReadAndWrite 0 //! any modifications enabled
#define ReadAndCreate 1 //! only adding new class members is allowed
#define ReadOnly 2 //! no modifications enabled
#define ReadOnlyVerified 3 //! no modifications enabled, CRC test applied
*/

#define Private		1
#define Corporal	2
#define Sergeant	3
#define Lieutenant 4
#define Captain		5
#define Major			6
#define Colonel		7
#define General		8

#define GTHeal		1
#define GTRepair	2
#define GTRefuel	3
#define GTReammo	4

// Product dependent options:

battleyeLicenceUrl = "BattlEye\EULA.txt";

class CfgMods
{
  defaultAction = "http://www.arma2.com/mods";
};

scriptsPath = "scripts\"; //"
startupScript = "";
cameraScript = "";
playerKilledScript = "onPlayerKilled.sqs";
playerResurrectScript = "onPlayerResurrect.sqs";

class CfgFunctions
{
  init = "";
};

#include "cfgSkill.hpp"
#include "cfgInventory.hpp"

maxBodiesAdded = 5;

#include "cfgMaterials.hpp"
#include "cfgMoves.hpp"
#include "cfgWeapons.hpp"
#include "cfgVehicles.hpp"
#include "cfgVoice.hpp"
#include "cfgCoreData.hpp"
#include "cfgFormations.hpp"
#include "cfgVideoOptions.hpp"


class CfgSurfaceCharacters
{
  // clutter 1 rock1, clutter 2 rock2, clutter 3 grass1, clutter 4 grass2
  class Empty
  {
    probability[]={};
    names[]={};
  };
};

/*!
\patch 1.43 Date 1/22/2002 by Ondra
- Fixed: CfgSurfaces protected against addon modification.
*/

class CfgSurfaces
{
	// caution: wildcard pattern "files" is matched using special purpose matching
	// * may be used only as last character of the pattern
	// * matches any string in filename
	// ? matches any single character in filename
	// when six trailing question-marks are detected "??????", two patterns are matched:
	// full pattern and short pattern with ?????? omitted
	class Default
	{
		access = ReadOnly;
		files=default;
		rough=0.075;
		dust=0.1;
		isWater=false;
        friction = 0.9;
        restitution = 0;
		soundEnviron = normalExt;
		character=Empty;
    visibility = 1;
    audibility = 1;
		impact = default_Mat;
	};
	class Water
	{
		access = ReadOnly;
		files=more_anim*;
		rough=0.0;
		dust=0.0;
		soundEnviron = water;
        friction = 0.9;
        restitution = 0;
		isWater=true;
		character=Empty;

		impact = default_Mat;
	};

};

class CfgDefaultSettings
{
	defaultVisibility = 8000;
	defaultTerrainGrid = 10;
	defaultObjectViewDistance = 1500;
	hazeDefaultDistance = -1;
	//default content of userInfo.cfg file for new users
	class UserInfo
	{
		//file version
		version=1;
		//file content
		blood=1;
		//viewDistance=8000;
		terrainGrid=10.000000;
		volumeCD=5;
		volumeFX=5;
		volumeSpeech=5;
		singleVoice=0;
		gamma=1.000000;
		brightness=1.000000;
		//fovTop=0.750000;
		//fovLeft=1.000000;
		//uiTopLeftX=0.075000;
		//uiTopLeftY=0.075000;
		//uiBottomRightX=0.925000;
		//uiBottomRightY=0.925000;
	};
};

/*!
\patch 2.03 Date 5/13/2003 by Ondra
- Fixed: CfgPatches protected against addon modification.
*/
class CfgPatches
{
	// new addons can create their own entries, but cannot modify existing ones
	access = ReadAndCreate;
};

#include "cfgFont.hpp"
#include "cfgUI.hpp"

class CfgDetectors
{
	objects[] = {EmptyDetector};
};

class CfgFaceWounds
{
	access = ReadAndCreate;
	wounds[]=
	{
		// \o\char\faceR01.paa,\o\char\faceR01_d.pac original, wounded, original, wounded...

		#include "woundsFace.hpp"

	};
};
class CfgGlasses 
{
  class None
  {
    scope = 2;
    name = "None";
    model = "";
    identityTypes[] = {};
  };
};
class CfgFaces
{
  class Default
  {
    class Default
    {
      name = "";
      head = "";
      material = "";
      texture = "#(rgb,8,8,3)color(0.5,0.5,1,1)";
      east = false; // TODO: replaced by identityTypes
      west = false; // TODO: replaced by identityTypes
      /// Filter - what units can use this face
      identityTypes[] = {"Default"};
    };
    class Custom
    {
      name = "";
      head = "";
      material = "";
      texture = "#(rgb,8,8,3)color(1,1,1,1)";
      front = "#(rgb,8,8,3)color(1,1,1,1)";
      side = "#(rgb,8,8,3)color(1,1,1,1)";
      back = "#(rgb,8,8,3)color(1,1,1,1)";
      east = false; // TODO: replaced by identityTypes
      west = false; // TODO: replaced by identityTypes
      /// Filter - what units can use this face (none for custom face)
      identityTypes[] = {};
    };
  };
};
class CfgMimics
{
	access = ReadAndCreate;
	class HeadPreview
	{
		microMimics=Micro;
		boneHead="head";
	};
	class Micro
	{
		//          max offset (3d)       change time (min..max)
		lBrow[] =  {0.0020,0.0020,0.0010, 0.6,3.0};
		mBrow[] =  {0.0020,0.0020,0.0010, 0.6,2.5};
		rBrow[] =  {0.0020,0.0020,0.0010, 0.6,3.0};
		lMouth[] = {0.0010,0.0020,0.0010, 0.6,2.0};
		mMouth[] = {0.0000,0.0000,0.0000, 0.6,3.0};
		rMouth[] = {0.0010,0.0020,0.0010, 0.6,2.0};
	};
	class NoMicro
	{
		//          max offset (3d)       change time (min..max)
		lBrow[] =  {0.0000,0.0000,0.0000, 0.6,3.0};
		mBrow[] =  {0.0000,0.0000,0.0000, 0.6,2.5};
		rBrow[] =  {0.0000,0.0000,0.0000, 0.6,3.0};
		lMouth[] = {0.0000,0.0000,0.0000, 0.6,2.0};
		mMouth[] = {0.0000,0.0000,0.0000, 0.6,3.0};
		rMouth[] = {0.0000,0.0000,0.0000, 0.6,2.0};
	};
	class States
	{
		class Default
		{
			// {right, up} in cm
			lBrow[] = {0, 0};
			mBrow[] = {0, 0};
			rBrow[] = {0, 0};
			lMouth[] = {0, 0};
			mMouth[] = {0, 0};
			rMouth[] = {0, 0};
		};
		class Normal
		{
			// {right, up} in cm
			lBrow[] = {0, 0};
			mBrow[] = {0, 0};
			rBrow[] = {0, 0};
			lMouth[] = {0, 0};
			mMouth[] = {0, 0};
			rMouth[] = {0, 0};
		};
		class Smile
		{
			// {right, up} in cm
			lBrow[] = {0, 0.25};
			mBrow[] = {0, 0.25};
			rBrow[] = {0, 0.25};
			lMouth[] = {-0.5, 0.6};
			mMouth[] = {0, 0.0};
			rMouth[] = {0.5, 0.6};
		};
		class Hurt
		{
			// {right, up} in cm
			lBrow[] = {0, -0.35};
			mBrow[] = {0, -0.5};
			rBrow[] = {0, -0.6};
			lMouth[] = {0.5, -0.3};
			mMouth[] = {0, 0.3};
			rMouth[] = {-0.5, -0.6};
		};
		class Ironic
		{
			// {right, up} in cm
			lBrow[] = {0, -0.35};
			mBrow[] = {0, -0.35};
			rBrow[] = {0, -0.35};
			lMouth[] = {0.1, 0.3};
			mMouth[] = {0, 0.9};
			rMouth[] = {-0.1, 0.3};
		};
		class Sad
		{
			// {right, up} in cm
			lBrow[] = {0, +0.2};
			mBrow[] = {0, +0.2};
			rBrow[] = {0, +0.2};
			lMouth[] = {0.1, -0.5};
			mMouth[] = {0, -0.5};
			rMouth[] = {-0.1, -0.5};
		};
		class Cynic
		{
			// {right, up} in cm
			lBrow[] = {0, -0.35};
			mBrow[] = {0, 0.0};
			rBrow[] = {0, +0.05};
			lMouth[] = {0.2, -0.9};
			mMouth[] = {0, -0.2};
			rMouth[] = {0.0, 0};
		};
		class Surprised
		{
			// {right, up} in cm
			lBrow[] = {0, 0.5};
			mBrow[] = {0, 0.5};
			rBrow[] = {0, 0.5};
			lMouth[] = {0.0, -0.3};
			mMouth[] = {0, +0.3};
			rMouth[] = {0.0, -0.2};
		};

		class Agresive
		{
			// {right, up} in cm
			lBrow[] = {0, -0.25};
			mBrow[] = {0, -0.4};
			rBrow[] = {0, -0.3};
			lMouth[] = {+0.4, +0.5};
			mMouth[] = {0, +0.5};
			rMouth[] = {-0.4, -0.5};
		};
		class Angry
		{
			// {right, up} in cm
			lBrow[] = {+0.4, -0.5};
			mBrow[] = {+0.1, -0.4};
			rBrow[] = {-0.4, -0.5};
			lMouth[] = {+0.4, -0.9};
			mMouth[] = {0, -0.2};
			rMouth[] = {-0.4, -0.8};
		};

	};

	combat[]=
	{
		Sad,0.1,
		Surprised,0.1,
		Cynic,0.3,
		Angry,0.1,
		Normal


	};
	aware[]=
	{
		Angry, 0.2,
		Normal

	};
	safe[]=
	{
		Smile,0.1,
		Surprised,0.1,
		Normal
	};
	neutral[]=
	{
		Surprised,0.1,
		Sad,0.1,
		Normal
	};
	danger[]=
	{
		Surprised,0.4,
		Sad
	};
	dead[]={Hurt};
	hurt[]={Hurt};
};

// define sound FX
class CfgEnvSounds
{
	class Default // editor - sounds from landscape
	{
		name = $STR_CFG_ENVSOUNDS_DEFAULT;
		sound[]={"$DEFAULT$",0,1};
		soundNight[]={"$DEFAULT$",0,1};
	};
	// list of environmental sounds for editor
	class Rain // rain
	{
		name = "";
		sound[]={,db-55,1};
		volume="rain";
	};
	class Sea // sea
	{
		name = "";
		sound[]={,db-70,1};
		volume="sea"; // audible even in rain
	};
	class Meadows // default - no trees, no sea, no hills ...
	{
		name = "";
		sound[]={,db-85,1};
		volume="meadow*(1-rain)*(1-night)"; // not audible in rain
	};
	class Trees // trees
	{
		name = "";
		sound[]={,db-75,1};
		volume="trees*(1-rain)*(1-night)"; // not audible in rain
	};
	class MeadowsNight // default - no trees, no sea, no hills ...
	{
		name = "";
		sound[]={,db-85,1};
		volume="meadow*(1-rain)*night"; // not audible in rain
	};
	class TreesNight // trees
	{
		name = "";
		sound[]={,db-75,1};
		volume="trees*(1-rain)*night"; // not audible in rain
	};
	class Hills // hills > 250m
	{
		name = "";
		sound[]={,db-75,1};
		volume="hills"; // not even in rain
	};
	class Wind
	{
		name = "";
		sound[]={,db-75,1};
		volume="(1-hills)*windy*0.5";
	};
};
class CfgHQIdentities
{
	access = ReadAndWrite;
	class Base
	{
		name = $STR_CFG_PAPABEAR;
		speaker = "NoVoice";
		pitch = 0.950000;
	};
	class HQ
	{
		name = $STR_CFG_PAPABEAR;
		speaker = "NoVoice";
		pitch = 1.050000;
	};
	class PAPA_BEAR
	{
		name = $STR_CFG_PAPABEAR;
		speaker = "NoVoice";
		pitch = 1.050000;
	};
	class AirBase
	{
		name = $STR_CFG_FIREFLYBASE;
		speaker = "NoVoice";
		pitch = 1.050000;
	};
};
class CfgHeads
{
	access = ReadAndCreate;
	class Air
	{
		friction=10;
		movement=160;
		maxAmp=0.05;
		maxSpeed=3;
		radius=0.2;
	};
	class Land
	{
		friction=20;
		movement=260;
		maxAmp=0.1;
		maxSpeed=4;
		radius=0.3;
	};
};

class CfgMusic
{
	access = ReadAndCreate;
/*
	class 7thLifeless
	{
		name = $STR_CFG_MUSIC_7TH_LIFELESS;
		sound[] = {\Music\Seventh_lifeless.ogg, db+0, 1.0};
	};
*/
};


class CfgSounds
{
	access = ReadAndCreate;

	// name, sound
/*
	class Fire
	{
		name = $STR_CFG_SOUNDS_FIRE;
		sound[] = {Objects\fire, db-72, 1.0};

		titles[] = {};
	};
*/
};

class CfgWhistleSound
{
  sound[]={,1,1};
  downTime = 0.5;
  silentTime = 1;
  upTime = 4;
};

class CfgTitles
{
	access = ReadAndCreate;
	defaultSpeed = 1;
	defaultIn = 0.5;
	defaultTime = 10;
	defaultOut = 0.5;
	titles[] =
	{
		Sphere, BISLogo, TVSet // UbiLogo,
		// Campaign, FirstBattle, NukeEden,
		// AttackKain, DefendEden, DefendAbel,
		// AttackEden
	};
	class Sphere
	{
		access = ReadAndWrite;
		name = $STR_CFG_TITLES_SPHERE;
		model = "koule.p3d";
		duration = 5;
		camera[] = {0,0,-10};
	};

	class BISLogo
	{
		access = ReadAndWrite;
		name = $STR_CFG_TITLES_BISLOGO;
		model = "bohemiaolor2.p3d";
		duration = 3;
		camera[] = {0,0,-200};
	};
	class TVSet
	{
		access = ReadAndWrite;
		name=$STR_CFG_TITLES_TVSET;
		duration=1000; //how to do unlimited time?
		model = "TVhi.p3d";
		camera[] = {0, 0,-1.2};
		//camera[] = {0,0,-0.91};
	};
};

class CfgIntro
{
};

class CfgCredits
{
	cutscene = "";
};

#define TitleTxt 0 // faded text
#define TitleTxtDown 1 // down faded text
#define TitleRsc 2 // resource text
#define TitleObj 3 // object

class CfgCutScenes
{
	access = ReadAndCreate;
	class Refuel
	{
		sound[]={"",0.5,0.2};
		titleType=TitleTxtDown;
		title=$STR_CFG_CUTSCENES_REFUEL;
	};
	class Rearm
	{
		sound[]={"",0.3,1};
		titleType=TitleTxtDown;
		title=$STR_CFG_CUTSCENES_REARM;
	};
	class Repair
	{
		sound[]={"",0.1,1};
		titleType=TitleTxtDown;
		title=$STR_CFG_CUTSCENES_REPAIR;
	};
};

#define CamExternal 0 // linked directly with object
#define CamInterpolated 0 // linked directly with object
#define CamStatic 1 // fixed point in space
#define CamStaticWithZoom 2 // fixed point in space
#define CamChained 3 // fixed point in space
#define CamTerminate 4 // fixed point in space
#define CamInternal 5 // fixed point in space

class CfgCameraEffects
{
	access = ReadAndCreate;
	class Interpolated
	{
		access = ReadAndWrite;
		type=CamInterpolated;
		scale=0.1;
		duration=5;
		spline=true; // spline or linear
		show=public;
	};
	class Exact: Interpolated
	{
		scale=-1;
	};
	class Chain
	{
		access = ReadAndWrite;
		type=CamChained;
		show=public;
	};

	class Array
	{
		class Terminate
		{
			access = ReadAndWrite;
			name=$STR_CFG_CAMEFFECTS_TERMINATE;
			type=CamTerminate;
			show=private;
		};
		class Internal
		{
			access = ReadAndWrite;
			name=$STR_CFG_CAMEFFECTS_INTERNAL;
			type=CamInternal;
			show=public;
		};
/* obsolote p3d with keyframe animation from OFP
		class ZoomIn: Interpolated
		{
			name=$STR_CFG_CAMEFFECTS_ZOOMIN;
			file="\anim\cam_zoom_in_front";
			scale=0.1;
			duration=5;
		};
		class ZoomInSlow: Interpolated
		{
			name=$STR_CFG_CAMEFFECTS_ZOOMINSLOW;
			file="\anim\cam_zoom_in_front";
			scale=0.1;
			duration=15;
		};
		class Around: Interpolated
		{
			name=$STR_CFG_CAMEFFECTS_AROUND;
			file="\anim\cam_360";
			duration=10;
			scale=0.3;
		};
		class AroundSlow: Interpolated
		{
			name=$STR_CFG_CAMEFFECTS_AROUNDSLOW;
			file="\anim\cam_360";
			duration=20;
			scale=0.3;
		};
		class ZoomAndAround: Chain
		{
			name=$STR_CFG_CAMEFFECTS_ZOOMAROUND;
			chain[]={ZoomIn,Around};
		};
		class AroundAndZoomINSlow: Chain
		{
			name=$STR_CFG_CAMEFFECTS_AROUNDZOOMSLOW;
			chain[]={AroundSlow, ZoomInSlow};
		};
		class ZoomInS: Interpolated
		{
			name=$STR_CFG_CAMEFFECTS_ZOOMINS;
			file="cam_s_zoom_in";
			duration=5;
			scale=1;
			show = private;
		};*/
		class Fixed: Interpolated
		{
			name=$STR_CFG_CAMEFFECTS_FIXED;
			type=CamStatic;
			duration=10;
		};
		class FixedWithZoom: Fixed
		{
			name=$STR_CFG_CAMEFFECTS_FIXEDZOOM;
			type=CamStaticWithZoom;
			duration=10;
		};
		class External: Fixed
		{
			name=$STR_CFG_CAMEFFECTS_EXTERNAL;
			type=CamExternal;
			duration=5;
			file="";
		};
	};
};

class CfgMarkers
{
	//access = ReadAndCreate;

	/*
	class Flag
	{
		name = $STR_CFG_MARKERS_FLAG;
		icon = "marker_objective.paa";
		color[] = {Red, 1};		//{Red, 0.8};
		size = SizeMapMarker;
	};
	*/
  class Empty
  {
    name = "empty";
    icon = ProcTextWhite;
    color[] = {0.2, 0.1, 0.4, 1};
    size = 0;
    scope = 2;
  };
};

class CfgMarkerColors
{
	//access = ReadAndCreate;
/*	class Default
	{
		name = $STR_CFG_MARKERCOL_DEFAULT;
		color[] = {Black, 1};	//{Black, 0.8};
	};
*/
};

class CfgMarkerBrushes
{
	//access = ReadAndCreate;
	class Solid
	{
		name = $STR_CFG_MARKERBR_SOLID;
		texture = ProcTextWhite;
    drawBorder = 0;
	}
};

class CfgLocationTypes
{
	class Mount
	{
		name = $STR_GETIN;
		drawStyle = mount; // icon, area, mount
		texture = ; // icon or brush
		color[] = {0, 0, 0, 1};
		size = 0; // size of icon (in pixels in resolution 640 x 480)
		font = FontMAIN;
		textSize = 0.04; // height of text (in screen coordinates)
    shadow = true;
	};
	// default key point name in map
	class Name
	{
		name = $STR_NAME;
		drawStyle = name;
		texture = ; // icon or brush
		color[] = {0, 0, 0, 1};
		size = 0; // size of icon
		font = FontMAIN;
		textSize = 0.04;
    shadow = true;
	};
	// key points used in map

	class Strategic: Name
	{
	  name = "Strategic key point"; //TODO: Stringtablize? (- will not be visible ingame)
		texture = ProcTextWhite;
		color[] = {0.25, 0.4, 0.2, 1};
		size = 16;
		textSize = 0.05;
	};

	class StrongpointArea: Strategic
	{
	  name = "Strongpoint area";//area where strongpoint (ie. bunker) can be placed  //TODO: Stringtablize? (- will not be visible ingame)
	};

	class FlatArea: Strategic
	{//main open flat spaces in landscape - detected as possible places for warfare bases, landing of helos etc.
	  name = "Flat area";  //TODO: Stringtablize? (- will not be visible ingame)
	};

	class FlatAreaCity: FlatArea
	{//main open flat space in city - used to place main city bunker (ie. fort, ie. supplydepot) in warfare
	  name = "Flat area in city";	  //TODO: Stringtablize? (- will not be visible ingame)
	};

	class FlatAreaCitySmall: FlatAreaCity
	{//smaller flat areas in city - can be used to place small buildings
	  name = "Flat area in city - small"; //TODO: Stringtablize? (- will not be visible ingame)
	};

	class CityCenter: Strategic
	{//true centers of villages or cities - used for seized calculations, this class will also have informations about neighboring cities (strategic graph)
	  name = "Center of the village/town/city"; //TODO: Stringtablize? (- will not be visible ingame)

	};

	class Airport: Strategic
	{
		name = "Airport"; //TODO: Stringtablize.
		texture = ProcTextWhite;
		importance = 20;
	};



	class NameMarine : Name
	{
		name = "$STR_NAME_MARINE";
		color[] = {0, 0.5, 0.95, 0.5};
		textSize = 0.05;
		importance = 3;
	};
	class NameCityCapital : Name
	{
		name = "$STR_NAME_CAPITAL";
		textSize = 0.07;
		importance = 7;
	};
	class NameCity : Name
	{
		name = "$STR_NAME_CITY";
		textSize = 0.06;
		importance = 6;
	};
	class NameVillage : Name
	{
		name = "$STR_NAME_VILLAGE";
		textSize = 0.05;
		importance = 5;
	};
	class NameLocal : Name
	{
		name = "$STR_NAME_LOCALNAME";
		color[] = {0, 0, 0, 0.5};
		textSize = 0.05;
		importance = 4;
	};

	class Hill : Name
	{
		name = "$STR_NAME_HILL";
		drawStyle = icon;
		texture = ProcTextWhite;
		color[] = {0, 0, 0, 1};
		size = 14;
		textSize = 0.04;
    		shadow = false;
		importance = 2;
	};
	class ViewPoint : Hill
	{
		name = "$STR_NAME_VIEWPOINT";
		drawStyle = icon;
		texture = ProcTextWhite;
		color[] = {0.78, 0, 0.05, 1};
		size = 12;
		textSize = 0.04;
    		shadow = false;
		importance = 1;
	};
	class RockArea : Hill
	{
		name = "$STR_NAME_ROCKAREA";
		texture = ProcTextWhite;
		color[] = {0, 0, 0, 1};
		size = 12;
    		shadow = false;
		importance = 2;
	};
	class BorderCrossing : Hill
	{
		name = "$STR_NAME_BCROSS";
		texture = ProcTextWhite;
		color[] = {0.78, 0, 0.05, 1};
		size = 16;
    		shadow = false;
		importance = 1;
	};
	class VegetationBroadleaf : Hill
	{
		name = "$STR_NAME_VEGB";
		texture = ProcTextWhite;
		color[] = {0.25, 0.4, 0.2, 1};
		size = 18;
    		shadow = false;
		importance = 2;
	};
	class VegetationFir : Hill
	{
		name = "$STR_NAME_VEGF";
		texture = ProcTextWhite;
		color[] = {0.25, 0.4, 0.2, 1};
		size = 18;
    		shadow = false;
		importance = 2;
	};
	class VegetationPalm : Hill
	{
		name = "$STR_NAME_VEGP";
		texture = ProcTextWhite;
		color[] = {0.25, 0.4, 0.2, 1};
		size = 18;
    		shadow = false;
		importance = 2;
	};
	class VegetationVineyard : Hill
	{
		name = "$STR_NAME_VEGV";
		texture = ProcTextWhite;
		color[] = {0.25, 0.4, 0.2, 1};
		size = 16;
    		shadow = false;
		importance = 2;
	};


};


#include "cfgWorlds.hpp"

class CfgGroups
{
};

class CfgAddons
{
	// addon configuration
	access = ReadAndCreate;
	// Following official addons should be always loaded if present
	// this should help to minimize compatibility problems
	// with older missions that do not have correct addons[] list
	// (This applies especially to mission using official weapons).
	class PreloadBanks
	{

	};
	class PreloadAddons
	{

	};
};

class CfgEditorObjects
{
  prefix_editor="bin\prefix.cpp";
  postfix_editor="bin\postfix.cpp";
  marker="bin\marker.cpp";
  script="bin\script.cpp";
  intel="bin\intel.cpp";
  unit="bin\unit.cpp";
  vehicle="bin\vehicle.cpp";
  item="bin\item.cpp";
};

#include "cfgXbox.hpp"
#include "resource.hpp"

class PreloadConfig
{
  // any texture corresponding to the entry below is preloaded
  // and kept in the memory permanently
  // actual texture file is looked up in the corresponding config entry
  // lookup values:
  //  "*" - the class including all subclasses is made permanent
  //  "." - only this class is made permanent
  // "fastFind" - optimize searching in this class
  RadioProtocolBase = "*";
  RadioProtocolDefault = "*";
  CfgVoice = "*";
  class CfgCloudlets
  {
    CraterBlood = ".";
    CloudletsMissileManual = "*";
    CloudletsMissile = "*";
    CloudletsScud = "*";
    CraterDustSmall = "*";
    CraterDustBig = "*";
    CraterSmoke1 = "*";
    CraterSmoke2 = "*";
    CraterSmoke3 = "*";
    CraterWater = "*";
    Explosion = "*";
  };
  CfgDestructPos="*";
  CfgCloudletShapes="*";
  CfgMimics = "*";
  CfgVehicles = "fastFind";
  CfgNonAIVehicles = "fastFind";
  CfgAmmo = "fastFind";
  CfgMusic = "*";
  CfgTitles = "*";
  CfgCameraEffects ="*";
  RscTitlesText="*";
  CfgFonts = "*";
  CfgFontFamilies = "*";

  // commanding menu
  RscMainMenu = "*";
  RscSubmenu = "*";
  RscMoveHigh = "*";
  RscMoveDir = "*";
  RscReply = "*";
  RscStatus = "*";
  RscWatchDir = "*";
  RscWatchMoreDir = "*";
  RscMoveDist = "*";
  RscFormations = "*";
  RscCombatMode = "*";
  RscTeam = "*";
  RscSelectTeam = "*";
  RscRadio = "*";
  // radio identities
  CfgHQIdentities = "*";
  RscButtonImages = "*";
  // in-game UI
  class RscInGameUI
  {
    RscUnitInfoSoldier = "*";
  };
  // objective icons
  RscObjectives = "*";
};

class CfgCameraShake
{
  /// coef used to multiply power to get positional change (in m)
  posChangeCoef = 0.005;
  /// coef used to multiply power to get rotation change in X (in degrees)
  rotXChangeCoef = 0.5;
  /// coef used to multiply power to get rotation change in Y (in degrees)
  rotYChangeCoef = 0.5;
  /// coef used to multiply power to get rotation change in Z (in degrees)
  rotZChangeCoef = 0.5;
  /// if linear interpolation should be used between 2 camera positions
  performLERP = true;
  /// if default values should be used when no values are present in config
  useDefaultValues = true;
  /// default power
  defaultPower = 12;
  /// default duration (in s)
  defaultDuration = 1.0;
  /// default max distance (in m)
  defaultMaxDistance = 120;
  /// default frequency of matrix update (number of updates per second)
  defaultFrequency = 20;
  /// default min speed of vehicle to cause shake
  defaultMinSpeed = 0;
  /// default min mass of vehicle to cause shake
  defaultMinMass = 10000;
  /// coef used to multiply caliber of ammo to get the coef for power and duration of shake used when the player is hit by ammo
  defaultCaliberCoefPlayerHit = 0.5;
  /// coef used to multiply caliber of ammo to get the coef for power and duration of shake used when the weapon is fired
  defaultCaliberCoefWeaponFire = 0.2;
  /// default coef used to multiply default power of shake of vehicles passing near player
  defaultPassingVehicleCoef = 0.05;
  /// time between two updates of shake of passing vehicles
  defaultPassingVehicleUpdateTime = 1.0;
  /// default vehicle shake attenuation coef
  defaultVehicleAttenuationCoef = 0.2;
};