#ifdef _MSC_VER
#pragma once
#endif

#ifndef _ARCADE_WAYPOINT_HPP
#define _ARCADE_WAYPOINT_HPP

//#include "wpch.hpp"
#include <Es/Strings/rString.hpp>
//#include "loadStream.hpp"
#include <El/Color/colors.hpp>
#include <El/ParamArchive/serializeClass.hpp>

#include "AI/pathPlanner.hpp"	// AI::Rank, AI::Formation, AI::Semaphore
#include "world.hpp"        // CamEffectPosition, TitEffectName

#include "objectId.hpp"

struct EFFECTS_INFO;
struct WAYPOINT_INFO;
struct MARKER_INFO;

enum TitleType
{
	TitleNone,
	TitleObject,
	TitleResource,
	TitleText
};

struct ArcadeEffects : public SerializeClass
{
	RString condition;

	RString sound;
	RString voice;
	RString soundEnv;
	RString soundDet;
	RString track;

	TitleType titleType;
	TitEffectName titleEffect;
	RString title;

	ArcadeEffects();
	ArcadeEffects(const ArcadeEffects &src);

	void Init();

	void FromNet(const EFFECTS_INFO &msg);
	void ToNet(EFFECTS_INFO &msg) const;

	LSError Serialize(ParamArchive &ar);
	LSError WorldSerialize(ParamArchive &ar);

  DECLARE_NETWORK_OBJECT
  static NetworkMessageFormat &CreateFormat
	(
		NetworkMessageClass cls,
		NetworkMessageFormat &format
	);
	TMError TransferMsg(NetworkMessageContext &ctx);
};


enum ArcadeUnitSpecial
{
	ASpNone,
	ASpCargo,
	ASpFlying,
	ASpForm,
  ASpCanCollide
};

enum ArcadeUnitAge
{
	AAActual,
	AA5Min,
	AA10Min,
	AA15Min,
	AA30Min,
	AA60Min,
	AA120Min,
	AAUnknown,
	AAN
};

enum ArcadeUnitPlayer
{
	APNonplayable,
	APPlayerCommander,
	APPlayerDriver,
	APPlayerGunner,
	APPlayableC,
	APPlayableD,
	APPlayableG,
	APPlayableCD,
	APPlayableCG,
	APPlayableDG,
	APPlayableCDG,
};

DEFINE_ENUM_BEG(LockState)
	LSUnlocked,
	LSDefault,
  LSLocked,
  LSLockedForPlayer
DEFINE_ENUM_END(LockState)

enum MarkerType
{
	MTIcon,
	MTRectangle,
	MTEllipse
};

#if _VBS2 // conditional markers
struct LayeredMarkerInfo : public RefCount
{
  Ref<Texture> icon;
	float a;
	float b;
	float x;
	float y;
  LayeredMarkerInfo() {icon = NULL; a = 0; b = 0; x = 0; y = 0;}
};
#endif

struct ArcadeMarkerInfo
{
	Point3 position;
	RString name;
	RString text;
	MarkerType markerType;
	RString type;
	RString colorName;
	PackedColor color;
  float alpha;
	Ref<Texture> icon;
	RString fillName;
	Ref<Texture> fill;
	float size;
	float a;
	float b;
	float angle;
  bool drawBorder;
  int shadow;

	bool selected;	

#if _VBS2 // conditional markers
  bool autosize;  // if true, marker will maintain size regardless of map zoom (default)
  RString attachCondition;  // for attaching to an object
  Ref<Object> attachTo;
  OLinkArray(Object) attachCtx;
  Vector3 attachOffset;  
  RString condition;        // for hiding/showing the marker
  bool isHidden;
  RefArray<LayeredMarkerInfo> layeredIcons;        // for layered markers
#endif

	ArcadeMarkerInfo();
	ArcadeMarkerInfo(const ArcadeMarkerInfo &src);

	void Init();

	LSError Serialize(ParamArchive &ar);

  DECLARE_NETWORK_OBJECT
  static NetworkMessageFormat &CreateFormat
  (
    NetworkMessageClass cls,
    NetworkMessageFormat &format
  );
	TMError TransferMsg(NetworkMessageContext &ctx);

	void AddOffset(Vector3Par offset);
	void Rotate(Vector3Par center, float angle, bool sel);
	void CalculateCenter(Vector3 &sum, int &count, bool sel);

	void OnColorChanged();
	void OnFillChanged();
	void OnTypeChanged();
};
TypeIsMovable(ArcadeMarkerInfo);

#endif
