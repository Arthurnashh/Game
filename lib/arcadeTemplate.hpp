#ifdef _MSC_VER
#pragma once
#endif

#ifndef _ARCADE_TEMPLATE_HPP
#define _ARCADE_TEMPLATE_HPP

#include "arcadeWaypoint.hpp"
#include "transport.hpp"

struct UNIT_INFO;
struct SENSOR_INFO;

#ifndef DECL_ENUM_TARGET_SIDE
#define DECL_ENUM_TARGET_SIDE
DECL_ENUM(TargetSide)
#endif

#include <Es/Containers/rStringArray.hpp>
#include "objectId.hpp"

struct ArcadeUnitInfo
{
	float presence;
	RString presenceCondition;
	Point3 position;
#if _VBS3 // mission.sqm positionASL
  Point3 positionASL;
#endif
	float placement;
	float azimut;
	ArcadeUnitSpecial special;
	ArcadeUnitAge age;
	int id;
	TargetSide side;
	RString vehicle;
	/// used to override type (in viewer)
	Ref<EntityAIType> type; 
	Ref<Texture> icon;
	float size;
	ArcadeUnitPlayer player;
  bool forceInServer; //true for role which should be always played on server by BOT_CLIENT
	bool leader;
	LockState lock;
	bool selected;
	Rank rank;
	float skill;
	float health;
	float fuel;
	float ammo;
  bool module;
	RString name;
	RString init;
	RString description; //!< Description of multiplayer role

	ArcadeUnitInfo();
	ArcadeUnitInfo(const ArcadeUnitInfo &src);

	void Init();

	void FromNet(const UNIT_INFO &msg);
	void ToNet(UNIT_INFO &msg) const;

	LSError Serialize(ParamArchive &ar);
	void AddOffset(Vector3Par offset);
	void Rotate(Vector3Par center, float angle, bool sel);
	void CalculateCenter(Vector3 &sum, int &count, bool sel);

	// ADDED in patch 1.01 - AddOns check
	void RequiredAddons(FindArrayRStringCI &addOns);
};
TypeIsMovable(ArcadeUnitInfo);

struct ArcadeIntel : public SerializeClass
{
	float friends[3][3];
	float weather;
	float fog;
  float wind;
	float weatherForecast;
  float fogForecast;
  float windForecast;
  float viewDistance;

	int year;
	int month;
	int day;
	int hour;
	int minute;

	RString briefingName;
	RString briefingDescription;

	ArcadeIntel();

	void Init();

	LSError Serialize(ParamArchive &ar);
};

struct ATSParams
{
	int nextVehId;
	bool avoidCheckSync;
  // hack to enable scripted missions to work in MP
  bool avoidScanAddons;
	ATSParams() 
  {
    nextVehId = 0; avoidCheckSync = false;
    // hack to enable scripted missions to work in MP
    avoidScanAddons = false;
  }
};

class Display;

struct IMission
{
  virtual int RandomSeed() const = 0;
  virtual ArcadeIntel Intel() const = 0;
  virtual void InitNoCenters(VehicleInitMessages &inits, GameMode mode) const = 0;
  virtual void ActivateAddons(RString worldname) = 0;
  virtual void InitGeneral(GameMode mode) = 0;
  virtual void AddMarkers() = 0;
  virtual FindArrayRStringCI AddOns() const = 0;
  virtual bool Load(RString name, RString cutscene, GameDataNamespace &globals, GameVarSpace &local) = 0;
  virtual bool IsEmpty() const = 0;
  virtual bool ParseMissionParams(RString missionParamsName, GameVarSpace &local) = 0;

  virtual ~IMission(){}
};

struct DummyMission : public IMission
{
  virtual int RandomSeed() const override {return 0;}
  virtual ArcadeIntel Intel() const override {return ArcadeIntel();}
  virtual void InitNoCenters( VehicleInitMessages &inits, GameMode mode ) const override {}
  virtual void ActivateAddons( RString worldname ) override {}
  virtual void InitGeneral(GameMode mode) override {}
  virtual void AddMarkers() override {}
  virtual FindArrayRStringCI AddOns() const override {return FindArrayRStringCI();}
  virtual bool Load( RString name, RString cutscene, GameDataNamespace &globals, GameVarSpace &local ) override {return false;}
  virtual bool IsEmpty() const override {return true;}
  virtual bool ParseMissionParams( RString missionParamsName, GameVarSpace &local ) override {return true;}
};

#if !USE_PRECOMPILATION
#error ScriptedMission requires precompilation.
#endif

struct MissionWithParams : public IMission
{
  //! mission designer can manually add into this section
  FindArrayRStringCI addOns;

  int randomSeed;
  ArcadeIntel intel;

  virtual bool ParseMissionParams(RString missionParamsName, GameVarSpace &local) override;
  virtual int RandomSeed() const override {return randomSeed;}
  virtual ArcadeIntel Intel() const override {return intel;}

  MissionWithParams();
};

struct ScriptedMission : public MissionWithParams
{
  RString filename;
  FindArrayRStringCI addons;
  bool loaded;

  ScriptedMission();
  virtual void InitNoCenters( VehicleInitMessages &inits, GameMode mode ) const override;
  virtual void ActivateAddons( RString worldname ) override;
  virtual void InitGeneral(GameMode mode) override;
  virtual void AddMarkers() override {}
  virtual FindArrayRStringCI AddOns() const override {return addons;}
  virtual bool Load( RString name, RString cutscene, GameDataNamespace &globals, GameVarSpace &local ) override;
  virtual bool IsEmpty() const override {return !loaded;}
};

struct ArcadeTemplate : public SerializeClass, public MissionWithParams
{
  virtual void InitNoCenters(VehicleInitMessages &inits, GameMode mode) const override;
  virtual void ActivateAddons(RString worldname) override;
  virtual void InitGeneral(GameMode mode) override;
  virtual void AddMarkers() override;
  virtual FindArrayRStringCI AddOns() const override {return addOns;}
  virtual bool Load(RString name, RString cutscene, GameDataNamespace &globals, GameVarSpace &local) override;
  virtual bool IsEmpty() const override;
  
	AutoArray<ArcadeUnitInfo> emptyVehicles;
	AutoArray<ArcadeMarkerInfo> markers;
	ArcadeIntel intel;
	int randomSeed;

	bool showHUD;
	bool showMap;
	bool showWatch;
	bool showCompass;
	bool showNotepad;
	bool showGPS;
	
	int nextVehId;

	//! list of addons added into addOns by mission editor - items in this list may be also automatically removed
	FindArrayRStringCI addOnsAuto;

	//! list of addons that were detected as missing
	FindArrayRStringCI missingAddOns;

	ArcadeTemplate();

	void ScanRequiredAddons(RString worldName);

	LSError Serialize(ParamArchive &ar);

	const ArcadeUnitInfo *FindPlayer() const;

	void Clear();

	void Compact();

	// ADDED in patch 1.01 - AddOns check
	void RequiredAddons(FindArrayRStringCI &addOns, RString worldName);
};
TypeIsMovable(ArcadeTemplate);

#endif
