#ifdef _MSC_VER
#pragma once
#endif

#ifndef _UI_ACTIONS_HPP
#define _UI_ACTIONS_HPP

#include "../Inventory/magazine.hpp"
#include "../Inventory/weapon.hpp"

//! action types
/*!
\patch_internal 1.01 Date 06/15/2001 by Jirka
- added ATUser action type
\patch 1.75 Date 3/8/2002 by Jirka
- Added: user actions assigned to vehicle type (ATUserType)
*/

#define ACTIONS_DEFAULT(XX) \
  XX(None, ActionBasic) \
  XX(GetInDriver, ActionBasic) \
  XX(GetInPilot, ActionBasic) \
  XX(GetInCargo, ActionIndex) \
  XX(GetInTurret, ActionTurret) \
  XX(Heal, ActionBasic) \
  XX(Repair, ActionBasic) \
  XX(Refuel, ActionBasic) \
  XX(GetOut, ActionBasic) \
  XX(LightOn, ActionBasic) \
  XX(LightOff, ActionBasic) \
  XX(EngineOn, ActionBasic) \
  XX(EngineOff, ActionBasic) \
  XX(SwitchMagazine, ActionWeaponIndex) \
  XX(UseWeapon, ActionWeaponIndex) \
  XX(LoadMagazine, ActionMagazineAndMuzzle) \
  XX(LoadOtherMagazine, ActionMagazineAndMuzzle) \
  XX(LoadEmptyMagazine, ActionMagazineAndMuzzle) \
  XX(TakeFlag, ActionBasic) \
  XX(ReturnFlag, ActionBasic) \
  XX(TurnIn, ActionBasic) \
  XX(TurnOut, ActionBasic) \
  XX(SitDown, ActionBasic) /* some special actions (can be used in safe mode) */ \
  XX(Land, ActionBasic) \
  XX(CancelLand, ActionBasic) \
  XX(Eject, ActionBasic) \
  XX(MoveToDriver, ActionBasic) \
  XX(MoveToPilot, ActionBasic) \
  XX(MoveToGunner, ActionBasic) \
  XX(MoveToCommander, ActionBasic) \
  XX(MoveToCargo, ActionIndex) \
  XX(MoveToTurret, ActionTurret) \
  XX(HideBody, ActionBasic) \
  XX(TouchOff, ActionBasic) \
  XX(StartTimer, ActionBasic) \
  XX(SetTimer, ActionBasic) \
  XX(Deactivate, ActionBasic) \
  XX(NVGoggles, ActionBasic) \
  XX(NVGogglesOff, ActionBasic) \
  XX(ManualFire, ActionBasic) \
  XX(ManualFireCancel, ActionBasic) \
  XX(AutoHover, ActionBasic) \
  XX(AutoHoverCancel, ActionBasic) \
  XX(StrokeFist, ActionBasic) \
  XX(StrokeGun, ActionBasic) \
  XX(LadderUp, ActionLadderPos) \
  XX(LadderDown, ActionLadderPos) \
  XX(LadderOnDown, ActionLadderPos) \
  XX(LadderOnUp, ActionLadderPos) \
  XX(LadderOff, ActionLadder) \
  XX(FireInflame, ActionBasic) \
  XX(FirePutDown, ActionBasic) \
  XX(LandGear, ActionBasic) \
  XX(LandGearUp, ActionBasic) \
  XX(FlapsDown, ActionBasic) \
  XX(FlapsUp, ActionBasic) \
  XX(Salute, ActionBasic) \
  XX(Taunt, ActionBasic) \
  XX(Clap, ActionBasic) \
  XX(Point, ActionBasic) \
  XX(ThumbsUp, ActionBasic) \
  XX(ScudLaunch, ActionBasic) \
  XX(ScudStart, ActionBasic) \
  XX(ScudCancel, ActionBasic) \
  XX(User, ActionIndex) \
  XX(UserType, ActionIndex) \
  XX(TakeMine, ActionBasic) \
  XX(DeactivateMine, ActionBasic) \
  XX(CancelAction, ActionBasic) \
  XX(Gear, ActionBasic) \
  XX(HealSoldier, ActionBasic) \
  XX(Surrender, ActionBasic)\
  XX(GetOver, ActionBasic) \
  XX(FirstAid, ActionBasic) \
  XX(IRLaserOn, ActionBasic) \
  XX(IRLaserOff, ActionBasic) \
  XX(GunLightOn, ActionBasic) \
  XX(GunLightOff, ActionBasic) \
  XX(ArtilleryComputer, ActionBasic) \
  XX(RepairVehicle, ActionBasic) \
  XX(TakeVehicleControl, ActionBasic) \
  XX(SuspendVehicleControl, ActionBasic) \
  XX(LockVehicleControl, ActionBasic) \
  XX(UnlockVehicleControl, ActionBasic) \
  XX(SearchLightOn, ActionBasic) \
  XX(SearchLightOff, ActionBasic) \
  XX(CollisionLightOn, ActionBasic) \
  XX(CollisionLightOff, ActionBasic) \
  XX(TakeItem, ActionBasic) \
  XX(DropItem, ActionBasic) \
  XX(PutItem, ActionMagazine) \
  XX(RadioOn, ActionBasic) \
  XX(RadioOff, ActionBasic) \
  XX(RadioVolumeUp, ActionBasic) \
  XX(RadioVolumeDown, ActionBasic) \
  XX(RadioNextChannel, ActionBasic) \
  XX(RadioPrevChannel, ActionBasic) \
  XX(TransmitterOn, ActionBasic) \
  XX(TransmitterOff, ActionBasic) \
  XX(TransmitterNextChannel, ActionBasic) \
  XX(TransmitterPrevChannel, ActionBasic) \
  XX(ObjectAction, ActionIndex) \
  XX(SurfaceAction, ActionSurface) \
  XX(Crafting, ActionCrafting) \
  XX(DetachAttachment, ActionIndex) \
  XX(TakeItemIntoHands, ActionBasic) \
  XX(Inspect, ActionBasic) \


#if _ENABLE_CONVERSATION
#define ACTIONS_CONVERSATION(XX) \
  XX(Talk, ActionBasic) \
  XX(Tell, ActionTell) \

#endif    

#if _HELISIM_ROTORLIB
#define ACTIONS_HELISIM(XX) \
  XX(EnginesOnAuto, ActionBasic) \
  XX(EnginesOffAuto, ActionBasic) \
  XX(HelicopterAutoTrimOn, ActionBasic) \
  XX(HelicopterAutoTrimOff, ActionBasic) \
  XX(HelicopterTrimOn, ActionBasic) \
  XX(HelicopterTrimOff, ActionBasic) \
  XX(BatteriesOn, ActionBasic) \
  XX(BatteriesOff, ActionBasic) \
  XX(APUOn, ActionBasic) \
  XX(APUOff, ActionBasic) \
  XX(RotorBrakeOn, ActionBasic) \
  XX(RotorBrakeOff, ActionBasic) \
  XX(StarterOn1, ActionBasic) \
  XX(StarterOff1, ActionBasic) \
  XX(StarterOn2, ActionBasic) \
  XX(StarterOff2, ActionBasic) \
  XX(StarterOn3, ActionBasic) \
  XX(StarterOff3, ActionBasic) \
  XX(ThrottleOff1, ActionBasic) \
  XX(ThrottleIdle1, ActionBasic) \
  XX(ThrottleFull1, ActionBasic) \
  XX(ThrottleOff2, ActionBasic) \
  XX(ThrottleIdle2, ActionBasic) \
  XX(ThrottleFull2, ActionBasic) \
  XX(ThrottleOff3, ActionBasic) \
  XX(ThrottleIdle3, ActionBasic) \
  XX(ThrottleFull3, ActionBasic) \
  XX(WheelsBrakeOn, ActionBasic) \
  XX(WheelsBrakeOff, ActionBasic) \

#endif

//!!  WARNING !!
//  if RTD action is added, change SelectedRotorLibActionChanged and SelectedRotorLibActionPerformed conditions //
//!!  WARNING !!

#if _VBS2
#define ACTIONS_VBS2(XX) \
  XX(InteractWithVehicle, ActionBasic) \
  XX(QuickEnterVehicle, ActionBasic)
#endif    

#define ACTIONS_ENUM(name, type) AT##name,

#ifndef DECL_ENUM_UI_ACTION_TYPE
#define DECL_ENUM_UI_ACTION_TYPE
DECL_ENUM(UIActionType)
#endif
DEFINE_ENUM_BEG(UIActionType)
  ACTIONS_DEFAULT(ACTIONS_ENUM)
#if _ENABLE_CONVERSATION
  ACTIONS_CONVERSATION(ACTIONS_ENUM)
#endif
#if _HELISIM_ROTORLIB
  ACTIONS_HELISIM(ACTIONS_ENUM)
#endif
#if _VBS3 && _ENABLE_CARRY_BODY //BattlefieldClearance
  ACTIONS_CARRY_BODY(ACTIONS_ENUM)
#endif
#if _VBS2
  ACTIONS_VBS2(ACTIONS_ENUM)
#endif
  NUIActionType
DEFINE_ENUM_END(UIActionType)

class InGameUI;
class INode;
typedef AutoArray<RefR<INode>, MemAllocLocal<RefR<INode>, 16> > UIActionParams;

/// basic action type
class Action : public RemoveLLinks
{
protected:
  UIActionType _type;
  OLink(Object) _target;
  bool _fromGUI; ///< true if action was create like a response on player action in GUI, false otherwise. 
  bool _processed; ///< helper variable set when the action was processed
#if _HELISIM_PRECISE_INTERACTION
  int _userActionID;
#endif

public:
#if _HELISIM_PRECISE_INTERACTION
  Action(UIActionType type) : _type(type) {_fromGUI = false; _processed = false; _userActionID = -1;}
  Action(UIActionType type, Object *target) : _type(type), _target(target) {_fromGUI = false; _processed = false; _userActionID = -1;}
  int GetUserActionID() {return _userActionID;};
#else
  Action(UIActionType type) : _type(type) {_fromGUI = false; _processed = false;}
  Action(UIActionType type, Object *target) : _type(type), _target(target) {_fromGUI = false; _processed = false;}
#endif

  virtual ~Action() {}

  UIActionType GetType() const {return _type;}
  Object *GetTarget() const {return _target;}
  void SetTarget(Object *target) {_target = target;}
  bool IsFromGUI() const {return _fromGUI;}
  void SetFromGUI(bool set) {_fromGUI = set;}
  bool IsProcessed() const {return _processed;}
  void SetProcessed(bool set) {_processed = set;}

  virtual bool HasEqualType(const Action *to) const;
  virtual int Compare(const Action *with) const; // for sorting
  bool IsEqual(const Action *to) const {return to && GetTarget() == to->GetTarget() && HasEqualType(to);}
  
  virtual Ref<Action> Copy() const = 0;

  virtual bool Process(AIBrain *unit) const;

  RefR<INode> GetText(AIBrain *unit) const;
  RefR<INode> GetTextDefault(AIBrain *unit) const;
  RString GetTextSimple(AIBrain *unit) const;
#if _HELISIM_PRECISE_INTERACTION
  RefR<INode> GetTextToolTip(AIBrain *unit) const;
#endif
  // helper for "action" scripting function
  virtual bool SetParams(const GameState *state, GameValuePar oper);

  // serialization of parameters to the network message
  virtual void SaveParams(QOStream &out) const = 0;
  virtual void LoadParams(QIStream &in) = 0;

#if _VBS3
  //executes a scripted action which is stores in the statement
  bool PerformScripted(AIBrain *unit) const;
#endif

  static Action *CreateObject(ParamArchive &ar);
  virtual LSError Serialize(ParamArchive &ar); // needed for ActivityUIAction

protected:
  virtual bool GetParams(UIActionParams &params, AIBrain *unit) const;
};

Action *CreateAction(UIActionType type);

#include <Es/Memory/normalNew.hpp>

/// action with no additional arguments
class ActionBasic : public Action
{
  typedef Action base;

public:
  ActionBasic(UIActionType type) : base(type) {}
  ActionBasic(UIActionType type, Object *target) : base(type, target) {}

  // Action implementation
  virtual Ref<Action> Copy() const;

  virtual bool GetParams(UIActionParams &params, AIBrain *unit) const;
  virtual bool Process(AIBrain *unit) const;

  virtual void SaveParams(QOStream &out) const override {}
  virtual void LoadParams(QIStream &in) override {}

  USE_FAST_ALLOCATOR
};

/// action referencing to turret
class ActionTurret : public Action
{
  typedef Action base;

protected:
  OLinkPermNO(Turret) _turret;

public:
  ActionTurret(UIActionType type) : base(type) {}
  ActionTurret(UIActionType type, Object *target, Turret *turret)
    : base(type, target), _turret(turret) {}

  Turret *GetTurret() const {return _turret;}

  // Action implementation
  virtual Ref<Action> Copy() const;

  virtual bool HasEqualType(const Action *to) const;
  virtual int Compare(const Action *with) const; // for sorting

  virtual bool GetParams(UIActionParams &params, AIBrain *unit) const;
  virtual bool Process(AIBrain *unit) const;

  virtual bool SetParams(const GameState *state, GameValuePar oper);

  virtual LSError Serialize(ParamArchive &ar); // needed for ActivityUIAction

  virtual void SaveParams(QOStream &out) const override;
  virtual void LoadParams(QIStream &in) override;

  USE_FAST_ALLOCATOR
};

/// action referencing to magazine slot
class ActionWeaponIndex : public Action
{
  typedef Action base;

protected:
  OLinkPermO(Person) _gunner;
  WeaponModePtr _index;

public:
  ActionWeaponIndex(UIActionType type) : base(type) {}
  ActionWeaponIndex(UIActionType type, Object *target, Person *gunner, const WeaponModePtr &index)
    : base(type, target), _gunner(gunner), _index(index) {}

  Person *GetGunner() const {return _gunner;}
  WeaponModePtr GetWeaponIndex() const {return _index;}

  // Action implementation
  virtual Ref<Action> Copy() const;

  virtual bool HasEqualType(const Action *to) const;
  virtual int Compare(const Action *with) const; // for sorting

  virtual bool GetParams(UIActionParams &params, AIBrain *unit) const;
  virtual bool Process(AIBrain *unit) const;

  virtual bool SetParams(const GameState *state, GameValuePar oper);

  virtual LSError Serialize(ParamArchive &ar); // needed for ActivityUIAction

  virtual void SaveParams(QOStream &out) const override;
  virtual void LoadParams(QIStream &in) override;

  USE_FAST_ALLOCATOR
};

/// action with generic index (used for user actions)
class ActionIndex : public Action
{
  typedef Action base;

protected:
  int _index;

public:

#if _HELISIM_PRECISE_INTERACTION
  ActionIndex(UIActionType type, int userID = -1) : base(type){_index = -1; _userActionID = userID;}
  ActionIndex(UIActionType type, Object *target, int index, int userID = -1) : base(type, target), _index(index) { _userActionID = userID;}
#else
  ActionIndex(UIActionType type) : base(type) {_index = -1;}
  ActionIndex(UIActionType type, Object *target, int index) : base(type, target), _index(index) {}
#endif

  int GetIndex() const {return _index;}
  void SetIndex(int index) { _index = index;}
  // Action implementation
  virtual Ref<Action> Copy() const;

  virtual bool HasEqualType(const Action *to) const;
  virtual int Compare(const Action *with) const; // for sorting

  virtual bool GetParams(UIActionParams &params, AIBrain *unit) const;
  virtual bool Process(AIBrain *unit) const;

  virtual bool SetParams(const GameState *state, GameValuePar oper);

  virtual LSError Serialize(ParamArchive &ar); // needed for ActivityUIAction

  virtual void SaveParams(QOStream &out) const override;
  virtual void LoadParams(QIStream &in) override;

  USE_FAST_ALLOCATOR
};

/// action with a surface description and an index
class ActionSurface : public Action
{
  typedef Action base;

protected:
  RString _surface;
  int _index;
  Vector3 _pos;

public:
  ActionSurface(UIActionType type) : base(type) {_index = -1;}
  ActionSurface(UIActionType type, RString surface, int index, Vector3Par pos) : base(type, nullptr), _surface(surface), _index(index), _pos(pos) {}

  RString GetSurface() const {return _surface;}
  int GetIndex() const {return _index;}
  void SetIndex(int index) {_index = index;}
  Vector3Val GetPos() const {return _pos;}
  // Action implementation
  virtual Ref<Action> Copy() const;

  virtual bool HasEqualType(const Action *to) const;
  virtual int Compare(const Action *with) const; // for sorting

  virtual bool GetParams(UIActionParams &params, AIBrain *unit) const;
  virtual bool Process(AIBrain *unit) const;

  virtual bool SetParams(const GameState *state, GameValuePar oper);

  virtual LSError Serialize(ParamArchive &ar); // needed for ActivityUIAction

  virtual void SaveParams(QOStream &out) const override;
  virtual void LoadParams(QIStream &in) override;

  USE_FAST_ALLOCATOR
};

/// action with a recipe invocation parameters
class ActionCrafting : public Action
{
  typedef Action base;

protected:
  RString _recipe;
  OLink(InventoryItem) _item;

public:
  ActionCrafting(UIActionType type) : base(type) {}
  ActionCrafting(UIActionType type, Object *target, InventoryItem *item, RString recipe) : base(type, target), _item(item), _recipe(recipe) {}

  InventoryItem *GetItem() const {return _item;}
  RString GetRecipe() const {return _recipe;}
  // Action implementation
  virtual Ref<Action> Copy() const;

  virtual bool HasEqualType(const Action *to) const;
  virtual int Compare(const Action *with) const; // for sorting

  virtual bool GetParams(UIActionParams &params, AIBrain *unit) const;
  virtual bool Process(AIBrain *unit) const;

  virtual bool SetParams(const GameState *state, GameValuePar oper);

  virtual LSError Serialize(ParamArchive &ar); // needed for ActivityUIAction

  virtual void SaveParams(QOStream &out) const override;
  virtual void LoadParams(QIStream &in) override;

  USE_FAST_ALLOCATOR
};

/// action referencing to magazine
class ActionMagazine : public Action
{
  typedef Action base;

protected:
  OLinkPermO(Person) _gunner;
  OLinkPermO(Magazine) _magazine;

public:
  ActionMagazine(UIActionType type) : base(type) {}
  ActionMagazine(UIActionType type, Object *target, Person *gunner, Magazine *magazine)
    : base(type, target), _gunner(gunner), _magazine(magazine) {}

  Person *GetGunner() const {return _gunner;}
  Magazine *GetMagazine() const {return _magazine;}

  // Action implementation
  virtual Ref<Action> Copy() const;

  virtual bool HasEqualType(const Action *to) const;
  virtual int Compare(const Action *with) const; // for sorting

  virtual bool GetParams(UIActionParams &params, AIBrain *unit) const;
  virtual bool Process(AIBrain *unit) const;

  virtual bool SetParams(const GameState *state, GameValuePar oper);

  virtual LSError Serialize(ParamArchive &ar); // needed for ActivityUIAction

  virtual void SaveParams(QOStream &out) const override;
  virtual void LoadParams(QIStream &in) override;

  USE_FAST_ALLOCATOR
};

/// action referencing to magazine and muzzle
class ActionMagazineAndMuzzle : public ActionMagazine
{
  typedef ActionMagazine base;

protected:
  Ref<const WeaponType> _weapon;
  Ref<const MuzzleType> _muzzle;

public:
  ActionMagazineAndMuzzle(UIActionType type) : base(type) {}
  ActionMagazineAndMuzzle(UIActionType type, Object *target, Person *gunner, Magazine *magazine, const WeaponType *weapon, const MuzzleType *muzzle)
    : base(type, target, gunner, magazine), _weapon(weapon), _muzzle(muzzle) {}

  const WeaponType *GetWeapon() const {return _weapon;}
  const MuzzleType *GetMuzzle() const {return _muzzle;}

  // Action implementation
  virtual Ref<Action> Copy() const;

  virtual bool HasEqualType(const Action *to) const;
  virtual int Compare(const Action *with) const; // for sorting

  virtual bool GetParams(UIActionParams &params, AIBrain *unit) const;
  virtual bool Process(AIBrain *unit) const;

  virtual bool SetParams(const GameState *state, GameValuePar oper);

  virtual LSError Serialize(ParamArchive &ar); // needed for ActivityUIAction

  virtual void SaveParams(QOStream &out) const override;
  virtual void LoadParams(QIStream &in) override;

  USE_FAST_ALLOCATOR
};

/// action referencing to ladder
class ActionLadder : public Action
{
  typedef Action base;

protected:
  int _index;

public:
  ActionLadder(UIActionType type) : base(type) {_index = -1;}
  ActionLadder(UIActionType type, Object *target, int index) : base(type, target), _index(index) {}

  int GetLadder() const {return _index;}

  // Action implementation
  virtual Ref<Action> Copy() const;

  virtual bool HasEqualType(const Action *to) const;
  virtual int Compare(const Action *with) const; // for sorting

  virtual bool GetParams(UIActionParams &params, AIBrain *unit) const;
  virtual bool Process(AIBrain *unit) const;

  virtual bool SetParams(const GameState *state, GameValuePar oper);

  virtual LSError Serialize(ParamArchive &ar); // needed for ActivityUIAction

  virtual void SaveParams(QOStream &out) const override;
  virtual void LoadParams(QIStream &in) override;

  USE_FAST_ALLOCATOR
};

/// action referencing to ladder position
class ActionLadderPos : public ActionLadder
{
  typedef ActionLadder base;

protected:
  int _pos;

public:
  ActionLadderPos(UIActionType type) : base(type) {_pos = -1;}
  ActionLadderPos(UIActionType type, Object *target, int index, int pos)
    : base(type, target, index), _pos(pos) {}

  int GetLadderPos() const {return _pos;}

  // Action implementation
  virtual Ref<Action> Copy() const;

  virtual bool HasEqualType(const Action *to) const;
  virtual int Compare(const Action *with) const; // for sorting

  virtual bool GetParams(UIActionParams &params, AIBrain *unit) const;
  virtual bool Process(AIBrain *unit) const;

  virtual bool SetParams(const GameState *state, GameValuePar oper);

  virtual LSError Serialize(ParamArchive &ar); // needed for ActivityUIAction

  virtual void SaveParams(QOStream &out) const override;
  virtual void LoadParams(QIStream &in) override;

  USE_FAST_ALLOCATOR
};

#if _ENABLE_CONVERSATION

class KBMessageInfo;

/// action referencing to sentence
class ActionTell : public Action
{
  typedef Action base;

protected:
  /// UI text
  RString _text;
  /// message to tell
  Ref<KBMessageInfo> _message;

public:
  ActionTell(UIActionType type) : base(type) {}
  ActionTell(UIActionType type, Object *target, RString text, KBMessageInfo *message)
    : base(type, target), _text(text), _message(message) {}

  RString GetText() const {return _text;}
  const KBMessageInfo *GetMessage() const {return _message;}

  // Action implementation
  virtual Ref<Action> Copy() const;

  virtual bool HasEqualType(const Action *to) const;
  virtual int Compare(const Action *with) const; // for sorting

  virtual bool GetParams(UIActionParams &params, AIBrain *unit) const;
  virtual bool Process(AIBrain *unit) const;

  virtual bool SetParams(const GameState *state, GameValuePar oper);

  virtual LSError Serialize(ParamArchive &ar); // needed for ActivityUIAction

  virtual void SaveParams(QOStream &out) const override;
  virtual void LoadParams(QIStream &in) override;

  USE_FAST_ALLOCATOR
};

#endif

#include <Es/Memory/debugNew.hpp>

class UIAction
{
public:
  /// action type and parameters  
  Ref<Action> action;

  // possible positions where action is available (indices of points in the memory LOD)
  AutoArray<int> _memoryPoints;

  // UI flags
  float priority;
  bool show;
  bool showWindow;
  bool hideOnUse;
  bool highlight; ///< give player notice of this action
  UserAction shortcut;
  UITime created;

#if _HELISIM_PRECISE_INTERACTION
  AutoArray<RString> modelPositions;
  float radius;
  float radiusView;
  int showIn3D;
  int available;
  RString textToolTip;
  int userActionID;
#endif

  UIAction();

  // action is needed now to decide type and 
  UIAction(Action *act);

  void Process(AIBrain *unit) const;

  bool IsEqual(const UIAction &to) const
  {
    return action->IsEqual(to.action);
  }
  bool HasEqualType(const UIAction &to) const
  {
    return action->HasEqualType(to.action);
  }

  bool operator == (const UIAction &with) const;
  bool operator != (const UIAction &with) const {return !operator ==(with);}

  NetworkMessageType GetNMType(NetworkMessageClass cls) const;
  static NetworkMessageFormat &CreateFormat(NetworkMessageClass cls, NetworkMessageFormat &format);
  TMError TransferMsg(NetworkMessageContext &ctx);
};
TypeIsMovable(UIAction)

/// description of action type, loaded from config
struct UIActionTypeDesc
{
  float priority;
  bool show;
  bool showWindow;
  bool hideOnUse;
  UserAction shortcut;
  RString text;
  RString textDefault;
  RString textSimple;

#if _HELISIM_PRECISE_INTERACTION
  AutoArray<RString> modelPositions;
  float radius;
  float radiusView;
  int showIn3D;
  int available;
  RString textToolTip;
  int userActionID;
#endif

#if _VBS3 //used for scripted actions
  RString script;
#endif

  UIActionTypeDesc();
  void Load(ParamEntryPar cls);
};

class UIActions : public StaticArray<UIAction>
{
  typedef StaticArray<UIAction> base;
public:
  
  static UIActionTypeDesc actionTypes[NUIActionType];
  static void Load(ParamEntryPar cls);

  void Sort();
};

class UIActionsDrawable : public UIActions
{
  typedef UIActions base;

protected:
  Ref<Action> _selected;
#if _HELISIM_PRECISE_INTERACTION 
  Ref<Action> _lastSelected;
#endif
  float _selectedPriority;

  AutoArray<UIAction> _newActions;
  bool _default;
  
  // default action parameters
  bool _showHint;
  bool _showNext;
  bool _showLine;
  Ref<Font> _defaultFont;
  float _defaultSize;
  PackedColor _defaultColor;
  Ref<Font> _nextFont;
  float _nextSize;
  float _xOffset;
  float _yOffset;
  float _xHotspot;
  float _yHotspot;
  float _relativeToCursor;
  Ref<Texture> _defaultBackground;
  int _defaultShadow;

#if _HELISIM_PRECISE_INTERACTION
  float _textTipFade;
  float _textTipDelay;
#endif
  // actions menu parameters
  /// how the menu is aligned
  int _align;
  /// position of the menu
  float _x, _y;
  /// maximal number of rows
  int _rows;

  PackedColor _bgColor, _textColor, _selBgColor, _selColor;
  Ref<Font> _font;
  float _size;
  Ref<Texture> _background;

  int _shadow;

  bool _underlineSelected;

  /// scrolling arrows
  float _arrowWidth, _arrowHeight;
  Ref <Texture> _iconArrowUp, _iconArrowDown;

  UITime _dimStart;

public:
  UIActionsDrawable();

  void SelectPrev(bool cycle);
  void SelectNext(bool cycle);
  void ProcessAction(int index, AIBrain *unit, bool fromGUI = false, bool byShortcut=false);

  void OnDraw();

  void Refresh(bool user);
  void Hide();
  float GetAge() const;
  float GetAlpha() const;
  bool IsDefault() const {return _default;}
  bool IsListVisible() const;

  void Update(UIActions &src);

  Ref<Action> GetSelected() {return _selected;};

  int FindSelected();
};

#endif
