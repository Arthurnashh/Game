#ifdef _MSC_VER
#pragma once
#endif

#ifndef _IN_GAME_UI_HPP
#define _IN_GAME_UI_HPP

#include "../types.hpp"
#include <El/Math/math3d.hpp>
#include "../AI/aiTypes.hpp"

//enum Team;
class EntityAIFull;
class Target;

class AIBrain;
class KBMessageInfo;
struct ConversationContext;

class INode;

class InventoryItemType;

#include "../vehicleAI.hpp"
class AbstractUI
{
protected:
	bool _showUnitInfo;
	bool _showCompass;
	bool _showTankDirection;
	bool _showCursors;

  float _weaponInfoAlpha;
  PackedColor _weaponInfoColor;

#if _HELISIM_PRECISE_INTERACTION
  UITime _textTipDelayTime;
#endif

public:
  /// disable / enable HUD drawing by the script
  bool _showHUD;

  /// draw anything into the 3d scene as needed
  virtual void Draw3D() = 0;
	/// draw overlay - called before FinishDraw
	virtual void DrawHUD(const Camera &camera, CameraType cam)= 0;

  /// draw hints, need to be called later then DrawHUD
  virtual void DrawHints ()= 0;

	/// simulate controls - called during simulation
	virtual void SimulateHUD(const Camera &camera, CameraType cam, float deltaT)= 0;

	virtual void DrawHUDNonAI(const Camera &camera, Entity *vehicle, CameraType cam)= 0;

	/// simulate controls - called during simulation
	virtual void SimulateHUDNonAI(const Camera &camera, Entity *vehicle, CameraType cam, float deltaT)= 0;

  /// limited in-game UI drawing when camera effect active
  virtual void DrawHUDCameraEffect(const Camera &camera, CameraType cam) = 0;

#if _HELISIM_PRECISE_INTERACTION
  UITime GetTextTipDelayTime() {return _textTipDelayTime;};
  void SetTextTipDelayTime(UITime time) {_textTipDelayTime = time;};
#endif

  //if hud was drawn in igui, do not draw head mounted display
  virtual bool IGUI_HMD() = 0;

#if _VBS3 // command mode in RTE
  virtual void ProcessMenu(const Camera &camera, CameraType cam, EntityAI *vehicle) = 0;
  virtual bool IsSelectedUnit(AIUnit *unit) const = 0;

  virtual void DrawMenu(bool isInEditor = false) = 0;
  virtual void DrawGroupInfo(EntityAI *vehicle) = 0;

  bool IsShownMenu() {return _showMenu;}
  bool IsShownGroupInfo() {return _showGroupInfo;}

  virtual void UpdateTMTime() = 0;
#endif

	// called in BrowseCamera - when HUD subject changes
	virtual void Init() = 0;
  // called when mission started / restarted
  virtual void InitMission() = 0;
	virtual void ResetHUD() = 0;
	virtual void ResetVehicle( EntityAIFull *vehicle ) = 0; // when vehicle is changed
  virtual void ResetUnitInfo() = 0;
  /// Preload textures needed to draw tank direction
  virtual void PreloadTankDirection() = 0;
  //reset weapon info alfa after reload
  virtual void ShowWeaponInfo() = 0;

  /// check what target is the cursor pointing at
  virtual Target *GetCursorTarget() const = 0;
  /// check what object is the cursor pointing at
  virtual Object *GetCursorObject() const = 0;
  /// check what surface is the cursor pointing at
  virtual RString GetCursorSurface() const = 0;

  /// Reveal target on cursor position
  virtual void RevealCursorTarget() = 0;

	void ShowUnitInfo(bool show = true) {_showUnitInfo = show;}
	void ShowCompass(bool show = true) {_showCompass = show;}
	void ShowTankDirection(bool show = true) {_showTankDirection = show;}
	void ShowAll(bool show = true);

  void ScriptShowHUD(bool show = true) {_showHUD = show;}
  bool ScriptIsHUDShown() const {return _showHUD;}

  bool IsHUDShown() const;
  virtual bool IsCompassShown() const = 0;
  virtual bool IsCommandingForced() const = 0;

	void ShowCursors(bool show = true) {_showCursors = show;}

	virtual void ShowHint(RString hint, bool sound) = 0;
  virtual void ShowHint(INode *hint, bool sound) = 0;

  virtual void ShowTaskHint(RString taskHint, PackedColor color, Texture *texture) = 0;
	
	// used to handle mouse movements
	virtual Vector3 GetCursorDirection() const = 0;
  virtual Vector3 GetCamGroupCameraPos() const = 0;
  virtual void InitCamGroupCameraPos() = 0;
  virtual void CamGroupActions(float deltaT) = 0;
	virtual void SetCursorDirection( Vector3Par dir ) = 0;

  virtual LONG ProcessUserMessage(UINT msg, WORD wParam, DWORD lParam) = 0;

  virtual Texture *GetCueTexture() const = 0;
  virtual PackedColor GetCueColor() const = 0;
  virtual PackedColor GetCueEnemyColor() const = 0;
  virtual PackedColor GetCueFriendlyColor() const = 0;
  virtual Texture *GetBloodTexture() const = 0;
  virtual PackedColor GetBloodColor() const = 0;
  virtual int GetCueShadow() const = 0;


  virtual Texture *GetHitZonesTexture() const = 0;
  virtual int const GetHitZonesShadow() const = 0;

  virtual bool IsXBOXStyle() = 0;

  //player artillery controls
#if _HELISIM_ROTORLIB
  virtual PackedColor GetHelicopterHUDColor() = 0;
#endif

#if _ENABLE_CONVERSATION
  /// return the context of conversation menu
  virtual ConversationContext *GetConversationContext() = 0;
  /// open the conversation menu
  virtual void ProcessConversation(AIBrain *askingUnit, AIBrain *askedUnit, const KBMessageInfo *question) = 0;
#endif

  virtual void UpdateActions(const AutoArray<UIAction> &myActions, const AutoArray<UIAction> &targetActions) = 0;

	virtual ~AbstractUI(){}
};

AbstractUI *CreateInGameUI();

#endif
