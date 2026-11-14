#ifdef _MSC_VER
#pragma once
#endif

#ifndef _KEY_INPUT_HPP
#define _KEY_INPUT_HPP

#include "inputInterfaceDef.h"
#include "global.hpp"

#include <El/ParamFile/paramFileDecl.hpp>
#include "paramFileExt.hpp"

#if _ENABLE_STEAM
# include <Steamworks/steam_api.h>

extern bool UseSteam;

/// Modification of Steam callbacks (CCallback, CCallbackManual) - do not call Unregister when nothing was registered (avoid dll call when not needed)
template< class T, class P, bool bGameServer >
class CCallbackEx : private CCallbackBase
{
public:
  typedef void (T::*func_t)( P* );

  // If you can't support constructing a callback with the correct parameters
  // then uncomment the empty constructor below and manually call
  // ::Register() for your object
  // Or, just call the regular constructor with (NULL, NULL)
  // CCallback() {}

  // constructor for initializing this object in owner's constructor
  CCallbackEx( T *pObj, func_t func ) : m_pObj( pObj ), m_Func( func )
  {
    if (pObj && func)
      Register( pObj, func );
  }

  ~CCallbackEx()
  {
    if (m_nCallbackFlags & k_ECallbackFlagsRegistered)
      Unregister();
  }

  // manual registration of the callback
  void Register( T *pObj, func_t func )
  {
    if (m_nCallbackFlags & k_ECallbackFlagsRegistered)
      Unregister();

    if (pObj && func)
    {
      m_nCallbackFlags |= k_ECallbackFlagsRegistered;
      if ( bGameServer )
      {
        m_nCallbackFlags |= k_ECallbackFlagsGameServer;
      }
      m_pObj = pObj;
      m_Func = func;
      SteamAPI_RegisterCallback( this, P::k_iCallback );
    }
  }

  void Unregister()
  {
    m_nCallbackFlags &= ~k_ECallbackFlagsRegistered;
    m_pObj = NULL;
    m_Func = NULL;
    SteamAPI_UnregisterCallback( this );
  }

private:
  virtual void Run( void *pvParam )
  {
    (m_pObj->*m_Func)( (P *)pvParam );
  }
  virtual void Run( void *pvParam, bool, SteamAPICall_t )
  {
    (m_pObj->*m_Func)( (P *)pvParam );
  }
  int GetCallbackSizeBytes()
  {
    return sizeof( P );
  }

  T *m_pObj;
  func_t m_Func;
};

// Allows you to defer registration of the callback
template< class T, class P, bool bGameServer >
class CCallbackExManual : public CCallbackEx< T, P, bGameServer >
{
public:
  CCallbackExManual() : CCallbackEx< T, P, bGameServer >( NULL, NULL ) {}
};

// utility macro for declaring the function and callback object together
#define STEAM_CALLBACK_EX( thisclass, func, param, var ) CCallbackEx< thisclass, param, false > var; void func( param *pParam )

// same as above, but lets you defer the callback binding by calling Register later
#define STEAM_CALLBACK_EX_MANUAL( thisclass, func, param, var ) CCallbackExManual< thisclass, param, false > var; void func( param *pParam )

#endif

#if _DISABLE_GUI
  #define ENABLE_KEYMOUSE 0
#else
  #define ENABLE_KEYMOUSE 1
#endif


//! action response curve function
class IActionResponseCurve: public RefCount
{
  public:
  // x in range 0..1
  virtual float Apply(float x, float zoom) const = 0;
  virtual float GetInverse(float y, float zoom) const = 0;

  // inverse including negative range
  float ApplyFullRange(float x, float zoom) const
  {
    return x>=0 ? Apply(x,zoom) : -Apply(-x,zoom);
  }
  // inverse including negative range
  float GetInverseFullRange(float y, float zoom) const
  {
    return y>=0 ? GetInverse(y,zoom) : -GetInverse(-y,zoom);
  }
  float ApplyWithDeadZone(float x, float deadzone, float zoom) const
  {
    //  x = (x-deadzone)*(1/(1-deadzone));
    if (fabs(x)<deadzone)
    {
      return Apply(0,zoom);
    }
    else if (x<0)
    {
      return -Apply((-x-deadzone)/(1-deadzone),zoom);
    }
    else
    {
      return Apply((x-deadzone)/(1-deadzone),zoom);
    }
  }
};

//! create a response curve from a config entry
IActionResponseCurve *CreateActionResponseCurve(const IParamArrayValue &entry);
//! create a response curve from a config entry
IActionResponseCurve *CreateActionResponseCurve(ParamEntryVal entry);
//! default response
IActionResponseCurve *CreateActionResponseCurveDefault();

//! list of keys assigned to the action
struct KeyList: public AutoArray<int>
{
  float _deadZone;
  Ref<IActionResponseCurve> _curve;

  KeyList();
  explicit KeyList(const IParamArrayValue & curve);

  void SetKeys(const AutoArray<int> &src)
  {
    AutoArray<int>::operator = (src);
  }
};

struct KeyListEx
{
  const KeyList *_listSimple;
  const KeyList *_listScheme;

  KeyListEx(const KeyList *listSimple, const KeyList *listScheme)
    : _listSimple(listSimple), _listScheme(listScheme) {}

  bool IsEmpty() const {return _listSimple == NULL && _listScheme == NULL;}
  int Size() const {return (_listSimple ? _listSimple->Size() : 0) + (_listScheme ? _listScheme->Size() : 0);}
  int operator [](int index) const
  {
    if (_listSimple)
    {
      int n = _listSimple->Size();
    return index < n ? _listSimple->Get(index) : _listScheme->Get(index - n);
  }
    return _listScheme->Get(index);
  }
};

//! user action description
struct UserActionDesc
{
  const char *name;
  const int &desc;
  bool useDoubleTapAndHold;

  //! create action declaration
  UserActionDesc(const char *n, const int &d);
};

#if _ENABLE_CHEATS

#define X_CHEAT_LIST(XX) \
  XX(FrameRate,"Show / hide frame rate",0) \
  XX(LowFpsDetect,"Low fps detection",0) \
  XX(TargetFpsEmulate,"Target fps emulation",0) \
  XX(ShowUI,"Show / hide UI",0) \
  XX(Immortality,"Toggle immortality",0) \
  XX(SafeArea,"Draw safe area",0) \
  XX(MapShowCost,"Map: draw cost/locks",0) \
  XX(MapShowUnits,"Map: draw units",0) \
  XX(MapShowTriggers,"Map: draw triggers",0) \
  XX(EndOfMission1,"End of mission #1",0) \
  XX(UnlockMissions,"Unlock missions",0) \
  XX(SaveGame,"Save game",0) \
  XX(Time,"Time acc/dec",0)  \
  XX(Group0End,"Other (1)",0) \
  \
  XX(UseCB,"Command Buffers",0) \
  XX(UseBG,"Background Rendering",0) \
  XX(Predication,"Predication",0) \
  XX(MJCPULimit,"MicroJobs CPU Limit",0) \
  XX(PruneMode,"Horizon Pruning ",0) \
  XX(EnableLights,"Enable Lights",0) \
  XX(Instancing,"Instancing",0) \
  XX(EnableSpotLightsFC,"Spot Lights Culling",0) \
  XX(LightThold,"Lights Thold",0) \
  XX(NoTex,"No textures",0) \
  XX(NoTexLoad,"Stop loading textures",0) \
  XX(NoShapeLoad,"Stop loading models",0) \
  XX(MipBias,"Texture mip bias",0) \
  XX(SceneComplex,"Scene complexity",0) \
  XX(Group1End,"Other (2)",0) \
  \
  XX(PerfBotCap,"Perf. bottleneck capture",0) \
  XX(PerfBotCapTH,"Perf. bottleneck thold",0) \
  XX(Lag,"MP Lag diagnostics",0) \
  XX(PerfCap,"Slow frame capture",0) \
  XX(PerfCapTH,"Slow frame thold",0) \
  XX(FpsLog,"Fps log on/off",0) \
  XX(PerfLog,"Perf log on/off",0) \
  XX(Wind,"Show wind",0) \
  XX(SoundDiag,"Sound Diags",0) \
  XX(NetworkDiag,"Network Diags",0) \
  XX(GeomDiags,"Geometry Diags",0) \
  XX(MemLimit,"Memory limit",0) \
  XX(MemFlush,"Flush memory",0) \
  XX(MemSample,"Memory stats",0) \
  XX(TexSample,"Texture stats",0) \
  XX(Crash,"Crash the game",0) \
  XX(Freeze,"Freeze the game",0) \
  XX(Shutdown,"Shutdown",0) \
  XX(Group2End,"Other (3)",0) \
  \
  XX(EnhDistance,"Enhanced view distance",0) \
  XX(DenseAmbient,"A lot of life",0) \
  XX(EditorObjects,"Unlock Objects in Editor",0) \
  XX(Group3End,"Other (4)",0)

const int CXTGroups = 4;
#define CHEAT_GROUPS(XX) XX(0) XX(1) XX(2) XX(3)

#else

/// must be ordered by cheat level
#define X_CHEAT_LIST(XX) \
  XX(EnhDistance,"Enhanced View Distance",0) \
  XX(DenseAmbient,"A lot of life",0) \
  XX(ShowUI,"Show / Hide UI",1) \
  XX(EndOfMission1,"Skip Mission",1) \
  XX(UnlockMissions,"Unlock Missions",1) \
  XX(Time,"Time accel",1) \
  XX(EditorObjects,"Unlock Objects in Editor",2) \
  XX(Lag,"MP Lag diagnostics",2) \
  XX(LowFpsDetect,"Low FPS Detection",2)

// XX(FrameRate,"Show / hide frame rate") \

// no groups
const int CXTGroups = 1;
#define CHEAT_GROUPS(XX) XX(0)
#define CXTGroup0End CXTN - 1

#endif


#define X_CHEAT_ENUM_ITEM(value,text,level) CXT##value,

#ifndef DECL_ENUM_CHEAT_X_TYPE
#define DECL_ENUM_CHEAT_X_TYPE
DECL_ENUM(CheatXType)
#endif
DEFINE_ENUM_BEG(CheatXType)
X_CHEAT_LIST(X_CHEAT_ENUM_ITEM)
CXTN,
DEFINE_ENUM_END(CheatXType)

extern const int CheatXLevels[];

//RMB_HOLD_CLICK_DBL 1 ... RMB implemented to differentiate between Click,DblClick and Hold
#define RMB_HOLD_CLICK_DBL  1
#define N_MOUSE_BUTTONS     8
#define N_MOUSE_AXES        4

/// number of mouse axes including the wheel
#define N_MOUSE_AXES_INC_Z  (N_MOUSE_AXES+2)

// enable usage of Xbox controller on PC
#define N_JOYSTICK_BUTTONS  32

// easy-to-read names of XBox controller buttons and axes

#define XBOX_A 0
#define XBOX_B 1
#define XBOX_X 2
#define XBOX_Y 3
#define XBOX_Up 4
#define XBOX_Down 5
#define XBOX_Left 6
#define XBOX_Right 7
#define XBOX_Start 8
#define XBOX_Back 9
#define XBOX_LeftBumper 10
#define XBOX_RightBumper 11
#define XBOX_LeftTrigger 12
#define XBOX_RightTrigger 13
#define XBOX_LeftThumb 14
#define XBOX_RightThumb 15
#define XBOX_LeftThumbXRight 16
#define XBOX_LeftThumbYUp 17
#define XBOX_RightThumbXRight 18
#define XBOX_RightThumbYUp 19
#define XBOX_LeftThumbXLeft 20
#define XBOX_LeftThumbYDown 21
#define XBOX_RightThumbXLeft 22
#define XBOX_RightThumbYDown 23

#define XBOX_NoController 24
#define XBOX_NewController 25
#define XBOX_ControllerRemoved 26 // active controller was removed

#define N_JOYSTICK_AXES     8 // 3 normal, 3 rotate, 2 sliders
#define N_JOYSTICK_POV      8 // pov - 8 state
#define N_TRACKIR_AXES      6 // pitch,roll,yaw, x,y,z
#define N_FREETRACK_AXES      6 // pitch,roll,yaw, x,y,z

#if _SPACEMOUSE
  #define N_SPACEMOUSE_AXES   6 // pitch,roll,yaw, x,y,z
  #define N_SPACEMOUSE_BUTTONS    8 // 8 normal
#endif

#define N_JOYSTICK_TYPES    3

#define INPUT_DEVICE_MASK       0x00ff0000
#define INPUT_DEVICE_KEYBOARD   0x00000000
#define INPUT_DEVICE_MOUSE      0x00010000 //! mouse button
#define INPUT_DEVICE_STICK      0x00020000 //! stick button
#define INPUT_DEVICE_STICK_AXIS 0x00030000 //! stick axis
#define INPUT_DEVICE_STICK_POV  0x00040000 //! stick pov switch
#define INPUT_DEVICE_XINPUT     0x00050000 //! XInput device
#define INPUT_DEVICE_TRACKIR    0x00080000 //! track IR
#define INPUT_DEVICE_MOUSE_AXIS 0x00100000 //! mouse axis
#define INPUT_DEVICE_SPECIAL_COMBINATIONS 0x00110000 //! keyboard + mouse (LCtrl + LMB)
#define KEYBOARD_DOUBLE_TAP_OFFSET  256
#define INPUT_KEY_MASK          0x000000ff
#define INPUT_CTRL_OFFSET           512  //no longer needed (any combinations are possible now. But it can be used in controls shortcuts[].
const unsigned KEYBOARD_COMBO_OFFSET = 16777216;
#define MOUSE_CLICK_OFFSET          128
#define MOUSE_DOUBLE_CLICK_OFFSET   256

//@{ specific mouse axes
#define INPUT_DEVICE_MOUSE_AXIS_UP (INPUT_DEVICE_MOUSE_AXIS+2)
#define INPUT_DEVICE_MOUSE_AXIS_DOWN (INPUT_DEVICE_MOUSE_AXIS+3)
#define INPUT_DEVICE_MOUSE_AXIS_LEFT (INPUT_DEVICE_MOUSE_AXIS+0)
#define INPUT_DEVICE_MOUSE_AXIS_RIGHT (INPUT_DEVICE_MOUSE_AXIS+1)
#define INPUT_DEVICE_MOUSE_WHEEL_UP (INPUT_DEVICE_MOUSE_AXIS+4)
#define INPUT_DEVICE_MOUSE_WHEEL_DOWN (INPUT_DEVICE_MOUSE_AXIS+5)
//@}

enum CheatCode
{
  CheatNone,
  CheatExportMap,
  CheatWinMission,
  CheatSaveGame,
  CheatCrash,
  CheatFreeze,
  CheatError,
  CheatFlush,
  CheatFps,
  CheatUnlockMissions,
  CheatUnlockArmory,
  CheatHost,
/*
//((TO_BE_REMOVED_IN_FINAL
  CheatShowVoNBubbles,
//))TO_BE_REMOVED_IN_FINAL
*/
#if _ENABLE_LIFE_CHEAT
  CheatImmortality,
#endif
};

const int TrackIRAxes = N_TRACKIR_AXES;
const int JoystickNAxes = N_JOYSTICK_AXES;
const int JoystickNButtons = N_JOYSTICK_BUTTONS;

#if _SPACEMOUSE
  const int SpaceMouseNAxes = N_SPACEMOUSE_AXES;
  const int SpaceMouseNButtons = N_SPACEMOUSE_BUTTONS;
#endif

struct ControllerSchemeItem
{
  RString name;
  bool yAxis;
};
TypeIsMovableZeroed(ControllerSchemeItem)

#define CONTROLLER_SENSITIVITY_ENUM(type, prefix, XX) \
  XX(type, prefix, Low) \
  XX(type, prefix, Medium) \
  XX(type, prefix, High)

#ifndef DECL_ENUM_CONTROLLER_SENSITIVITY
#define DECL_ENUM_CONTROLLER_SENSITIVITY
DECL_ENUM(ControllerSensitivity)
#endif

// call to enum factory using this macro:
DECLARE_ENUM(ControllerSensitivity, CS, CONTROLLER_SENSITIVITY_ENUM);

struct ControllerScheme
{
  RString basicSettings;
  AutoArray<ControllerSchemeItem> settings;
  ControllerSensitivity sensitivity;
  // vibrations controlled by the Xbox Guide Settings on Xbox 360
  bool vibrations;
};

/// value together with corresponding curve
struct ValueWithCurve
{
protected:
  /// curve magnifier
  float _factor;
public:
  /// source value for the curve
  float _value;
  /// curve center point
  float _center;
  /// curve
  IActionResponseCurve *_curve;

  ValueWithCurve(float value, IActionResponseCurve *curve);
  ValueWithCurve(float value, float center, float factor, IActionResponseCurve *curve)
    : _value(value), _center(center), _factor(factor), _curve(curve) {}
  float GetAnyValue(float value, float zoom=1) const;
  float GetValue(float zoom=1) const
  {
    return GetAnyValue(_value,zoom);
  }
  void MultFactor(float mult) { _factor *= mult; } 
  ValueWithCurve operator*(float mult)
  {
    return ValueWithCurve(_value, _center, _factor*mult, _curve);
  }
  float GetFactor() const { return _factor; }
};

typedef KeyList KeysMapping[UAN];

struct KeysScheme
{
  KeysMapping mapping[N_JOYSTICK_TYPES];
  RStringB name;
};
TypeIsMovable(KeysScheme)

struct KeyImage
{
  int id; // key id
  RString image;
  RString size;

  KeyImage()
  {
    id = -1;
    size = RString();
  }
};
TypeIsMovable(KeyImage)

struct KeyImageTraits: public DefMapClassTraits<KeyImage>
{
  /// key type
  typedef int KeyType;
  /// calculate hash value
  static unsigned int CalculateHashValue(KeyType key)
  {
    return key;
  }

  /// compare keys, return negative when k1<k2, positive when k1>k2, zero when equal
  static int CmpKey(KeyType k1, KeyType k2)
  {
    return k1 - k2;
  }

  /// get a key for given a item
  static KeyType GetKey(const KeyImage &item) {return item.id;}
};

typedef MapStringToClass<KeyImage, AutoArray<KeyImage>, KeyImageTraits> KeyImages;

class InputCollisions
{
private:
  struct ActionGroup
  {
    RString name;
    AutoArray<int> members;
    ActionGroup() {}
    ActionGroup(RString _name) { name=_name; }
    ActionGroup & operator=(const ActionGroup &val)
    {
      name = val.name;
      members = val.members;
      return *this;
    }
    ClassIsMovable(ActionGroup);
  };
  AutoArray<ActionGroup> actionGroups;
  AutoArray<ActionGroup> collisionGroups;
  AutoArray<ActionGroup> exceptions;
public:
  void Init();
  bool Collide(int act1, int act2);
};

class ActionGroups
{
private:
  struct ActionGroup
  {
    RString name;
    AutoArray<int> members;
    ActionGroup() {}
    ActionGroup(RString _name) { name=_name; }
    ActionGroup & operator=(const ActionGroup &val)
    {
      name = val.name;
      members = val.members;
      return *this;
    }
    ClassIsMovable(ActionGroup);
  };
  AutoArray<ActionGroup> actionGroups;
public:
  void Init();
  int Size() { return actionGroups.Size(); }
  RString GroupName(int ix) { if (ix<Size()) return actionGroups[ix].name; else return ""; }
  int GroupSize(int ix) { if (ix<Size()) return actionGroups[ix].members.Size(); else return 0; }
  const AutoArray<int> &Group(int ix) { return actionGroups[ix].members; }
};

class KeyCombination
{
public:
  int dik;
  int val; //this value is decremented in CombinationWanted::Update. When zero, dik is removed
  KeyCombination() : dik(0), val(0) {}
  KeyCombination(int setdik, int setval) : dik(setdik), val(setval) {}
  ClassIsMovableZeroed(KeyCombination);
};

class CombinationsWanted
{
private:
  AutoArray<KeyCombination> combinationsWanted[2];
  int curIx;

public:
  CombinationsWanted() : curIx(0) {}
  void Update();
  void AddCombination(int dik); 
  /// checkDik returns false when combination was wanted and combo key is pressed -> single key cannot pass the filter
  bool CheckDik(int dikKey);
};

/// input is closed within the frame scope if it is read with exclusive flag
struct ClosedInputItem
{
  int _key;
  unsigned char _level;

  ClosedInputItem() : _key(0), _level(0) {}
  ClosedInputItem(int key, unsigned char level) : _key(key), _level(level) {}

  bool operator == (const ClosedInputItem &item2) const
  {
    return _key == item2._key && _level == item2._level;
  }
};
TypeIsSimple(ClosedInputItem);

/// Blocked Input is blocked until the button is released (so also outside the frame scope)
struct BlockedInputItem
{
  int _key;
  bool _blocked;

  BlockedInputItem() : _key(0), _blocked(false) {}
  BlockedInputItem(int key) : _key(key), _blocked(true) {}

  bool operator == (const BlockedInputItem &item2) const
  {
    return _key == item2._key;
  }
};
TypeIsSimple(BlockedInputItem);

struct KeyProfile: public RefCount
{
  // functor for loading all actions from the config class
  class LoadKeyProfile
  {
    KeyProfile *_profile;
    public:
    LoadKeyProfile(KeyProfile *profile);
    bool operator () (ParamEntryPar entry) const;
  };

  RString name;
  KeysMapping keys;
  bool isDefault;

  bool Load(ConstParamEntryPtr parClass);
  bool IsDefault() const {return isDefault;}
};

typedef RefArray<KeyProfile> KeyProfileList;
class ProfilesManager
{
  KeyProfileList profiles;
  // index to profiles
  int currentIndex;
  // link to default profile
  Ref<KeyProfile> defaultProfile;

public:
  ProfilesManager();

  /// getting profile profile
  // gets currently selected profile
  Ref<KeyProfile> GetCurrentProfile();
  // gets profile by index
  Ref<KeyProfile> GetProfile(int index);
  // gets profile by name
  Ref<KeyProfile> GetProfile(RString name);

  /// selecting profile
  // setting active profile
  bool SetProfile(int index);
  bool SetProfile(RString name);
  // sets default profile as currently selected
  bool SetDefaultProfile();

  // TODO: for full management, implement remove functions

  /// getting key list from profile
  // getting values from profile
  KeyList GetKeyList(int profileIndex, UserAction action);
  // getting values from currently selected profile
  KeyList GetCurrentProfileKeyList(UserAction action);

  /// list all profiles to given array
  void ListProfiles(KeyProfileList &outList);

  // loading profiles
  void LoadProfiles(ConstParamEntryPtr ptr, bool clear = true);
};

/// Used to store DefaultMouseCurveDefs (same as KeyList, but no keys)
struct ResponseCurve
{
  float _deadZone;
  Ref<IActionResponseCurve> _curve;
  ResponseCurve();
};

enum JoystickMode {JMCustom, JMDisabled};

struct Input: public InputInterface
{
  // map actions to input events
  static UserActionDesc userActionDesc[UAN];

  ProfilesManager defaultProfiles;

  // input collisions groups
  InputCollisions inputCollisions;
  ActionGroups actionGroups;

  KeysMapping userKeys;
  KeysMapping backupJoysticksKeys;
  ResponseCurve mouseCurves[UAN];
  mutable KeyList tempKeyList;
  AutoArray<KeysScheme> keysSchemes;
  KeyListEx GetUserKeys(int index) const;

  bool actionDone[UAN];

  // keyboard
#if _ENABLE_CHEATS
  bool cheat1;
  bool cheat2;
  bool cheat12enabled;
  bool cheatVoNServer; //VoN force SERVER retranslation
  bool cheatVoNRetranslate; //VoN retranslate through GameSocket
  bool cheatVoNDetectNatType; //VoN Detect NAT type
  bool cheatVoNShow; //VoN Show the state of chatVoNxxx variables (using DIAG_MESSAGE)
#endif

  signed char cheatXToDo[CXTN];
  /// some XBox cheat may be a result of direct LT+RB+button combination
  bool cheatXDirect[N_JOYSTICK_BUTTONS];

  KeyImages keyImages;

  DWORD keyPressed[256]; // time stamp of last key pressed event
  DWORD keyLastPressed[256]; // time stamp of last key pressed event (not cleared after release)
  DWORD keyReleased[256]; // time stamp of last key released event
  bool keyKeepAlive[256]; // true, when released button is unreleased for some time quantum
  mutable bool keyDoubleClickWanted[256]; // true, when some action read the double click in the last frame
  mutable bool keyDoubleClickAndHold[256]; // true, when continuing hold after double tap should be considered as double tap itself again and again
  DWORD keyAutorepeat[256]; // time stamp of last handled autorepeat
  float keys[256]; // integrated key time (as part of deltaT)
  mutable CombinationsWanted combinationsWanted; //contains values, when some action read some key combinations in the last frame
  bool keysDoubleTapToDo[256]; // edges to process (double tap)
  bool keysToDo[256]; // edges to process (pressed)
  mutable unsigned char keysMaxLevelThisFrame[256]; // at which maximal level keysToDo was asked this frame
  unsigned char keysMaxLevelLastFrame[256]; // at which maximal level keysToDo was asked last frame
  bool keysUp[256]; // edges to process (released)
  bool keysProcessed[256]; // key event caught by UI
  bool rControlActive;
  mutable FindArrayKey<ClosedInputItem> _closedInput;
  mutable FindArrayKey<BlockedInputItem> _blockedInput; //blocked until the key is released!
  bool rmbClickClosed;

  bool awaitCheat;
  CheatCode cheatActive;
  RString cheatInProgress;

  DWORD delayAutorepeatFirst;
  DWORD delayAutorepeat;

  // special action locking
  float floatingZoneArea;

  // Voice over Net toggle status
  bool VoNToggleOn;

  /// on PC, defines if presets (scheme) or custom configuration (userKeys) is used
  bool customScheme;

  ControllerScheme scheme;

  void ProcessKeyPressed(int dik); // cheat processing
  void DisableKey(int key);

  CheatCode CheatActivated() const {return cheatActive;}
  void CheatServed() {cheatActive=CheatNone;}

  static bool IsMouseRangeFactorEater(UserAction action);

  /// frame identifier (used to call KeyboardPilots only once per frame)
  int frameID;
  // mouse
  float mouseButtons[N_MOUSE_BUTTONS];
  float mouseButtonsHold[N_MOUSE_BUTTONS];
  float mouseButtonsWithCtrl[N_MOUSE_BUTTONS]; // DIK_LCONTROL was pressed simultaneously
  unsigned int mouseButtonsToDo[N_MOUSE_BUTTONS];
  
  // at which maximal level mouseButtonsToDo was asked this frame
  mutable unsigned char mouseMaxLevelThisFrame[N_MOUSE_BUTTONS];
  // at which maximal level mouseButtonsToDo was asked last frame
  unsigned char mouseMaxLevelLastFrame[N_MOUSE_BUTTONS];
  
  /// at which maximal level mouseAxis was asked this frame (including mouse wheel)
  mutable unsigned char mouseAxisMaxLevelThisFrame[N_MOUSE_AXES_INC_Z];
  /// at which maximal level mouseAxis was asked last frame (including mouse wheel)
  unsigned char mouseAxisMaxLevelLastFrame[N_MOUSE_AXES_INC_Z];
  
  bool mouseButtonsClickToDo[N_MOUSE_BUTTONS];
  // RMB click can be set sometimes immediately after click but in some conditions also later when the button is released
  bool mouseButtonsClickToDoAlreadySet[N_MOUSE_BUTTONS];
  unsigned int mouseButtonsReleased[N_MOUSE_BUTTONS];
  DWORD mouseButtonsPressed[N_MOUSE_BUTTONS]; // time stamps of last button pressed event
  DWORD mouseButtonsReleasedTime[N_MOUSE_BUTTONS]; // time stamps of last button released event
  bool mouseButtonsDblClickDetected[N_MOUSE_BUTTONS]; // true when current button press was detected as dblClick
  bool mouseButtonsKeepAlive[N_MOUSE_BUTTONS]; // true, when released button is unreleased for some time quantum
  mutable bool mouseButtonDoubleClickWanted[N_MOUSE_BUTTONS]; // true, when some action read the double click in the last frame
  mutable bool mouseButtonClickWanted[N_MOUSE_BUTTONS]; // true, when some action read the click in the last frame
  mutable bool mouseButtonHoldWanted[N_MOUSE_BUTTONS]; // true, when some action read the hold in the last frame
  unsigned int mouseButtonsDoubleToDo[N_MOUSE_BUTTONS]; // edges to process (double click)
  int mouseZ; //mouseX, mouseY doesn't exist, as mouse movement is simulated as analog device (mouse speed)
  float mouseAxis[N_MOUSE_AXES];
  float mouseWheelUp, mouseWheelDown;
  bool mouseAxisCleared[N_MOUSE_AXES_INC_Z];
  static float MouseAxisThresholdHigh, MouseAxisThresholdLow;
  static const float MouseRangeFactor;
  bool mouseL,mouseR,mouseM;
  bool mouseLToDo,mouseRToDo,mouseMToDo; // click event
  bool mouseDClickToDo; // double click event
  unsigned long _lastProcessMouse;
  unsigned int mouseDoubleClickTime, mouseClickTime;
  float mouseSensitivityX;
  float mouseSensitivityY;
  /// separate gamma for each axis direction
  float joystickGamma[N_JOYSTICK_AXES*2];  // obsolete
  float joystickSensitivity[N_JOYSTICK_AXES*2];

  float GetMouseSensitivityMultX() {return 4.0f;}
  float GetMouseSensitivityMultY() {return 4.0f;}

  float cursorMovedZ;
  float cursorX,cursorY; // accumulate cursor position (-1..+1)
  // mouse - average values
  struct MouseVal
  {
    float deltaT;
    int offset[2];
    //float speed[2];;
    MouseVal(float dT=0, int x=0, int y=0) : deltaT(dT)
    {
      offset[0] = x;
      offset[1] = y;
    }
  };
  static const int MouseBufValSize=100;
  MouseVal _mouseBufVal[MouseBufValSize];
  /// current value used in _mouseBufVal
  int      _mouseBufValIx;
  /// filtering barrier for x/y axis of _mouseBufVal - never sample before this one
  int      _mouseBarrierIx[2];
  
  void GetAverage(float &speedX, float &speedY, int offsetX, int offsetY, int timeDelta);

  /// mouse filtering threshold  
  int _filterMouse;

  /// default value for _filterMouse
  static const int _defFilterMouse;
  
  float FilterMouseAxis(int axis, int offset, float duration);
  // stick
  struct JoystickState
  {
    int  offset;   //joystick action offset to map Action to Stick Input (multiple of JoysticksDevices::OffsetStep)
    AutoArray<float> sensitivity;
    AutoArray<float> deadZone;
    AutoArray<float> steepness;
    JoystickMode mode;
    bool isXInput;
    bool stickPov[N_JOYSTICK_POV];
    bool stickPovOld[N_JOYSTICK_POV]; // edge detection
    bool stickPovToDo[N_JOYSTICK_POV];

    float stickButtons[N_JOYSTICK_BUTTONS];
    bool stickButtonsToDo[N_JOYSTICK_BUTTONS];
    mutable unsigned char stickMaxLevelThisFrame[N_JOYSTICK_BUTTONS]; // at which maximal level stickButtonsToDo was asked this frame
    unsigned char stickMaxLevelLastFrame[N_JOYSTICK_BUTTONS]; // at which maximal level stickButtonsToDo was asked last frame
    bool stickButtonsPressed[N_JOYSTICK_BUTTONS];
    bool stickButtonsUp[N_JOYSTICK_BUTTONS];
    bool stickAutorepeat[N_JOYSTICK_BUTTONS];
    UITime stickAutorepeatTime[N_JOYSTICK_BUTTONS];

    float stickAxis[N_JOYSTICK_AXES];
    bool stickAxisCleared[2*N_JOYSTICK_AXES];
    bool stickPOVCleared[N_JOYSTICK_POV];
    static float StickAxisThresholdHigh, StickAxisThresholdLow;

    /*!
    \name Stick movement detection
    */
    //! last axis state
    int jAxisLast[JoystickNAxes];
    //! time of last axis movement
    UITime jAxisLastActive[JoystickNAxes];

    JoystickState()
    {
      mode = JMCustom;
      isXInput = false;
      ClearJoystickVariables();
    }

    void ClearJoystickVariables()
    {
      for (int i=0; i<N_JOYSTICK_BUTTONS; i++)
      {
        stickMaxLevelThisFrame[i] = 0;
        stickMaxLevelLastFrame[i] = 0;
      }
      //stick axis and POV
      for (int i=0; i<2*N_JOYSTICK_AXES; i++) stickAxisCleared[i]=true;
      for (int i=0; i<N_JOYSTICK_POV; i++) stickPOVCleared[i]=true;
    }

    bool IsEnabled() const;

    ClassIsMovableZeroed(JoystickState);
  };

  void ApplySensitivity(float &val, float sVal, float steepVal)
  {
    if (sVal < 0)
    { // interpolate between linear and cubic
      sVal = -sVal;
      val = sVal*val*val*val + (1-sVal)*val;
    }
    else
    { // interpolate between linear and val^(1/3)
      val = sVal*pow(val,1/3.0f) + (1-sVal)*val;
    }
    // apply steepness
    val *= steepVal;
    saturate(val, 0, 1.0f);
  }

private:
  void SaveJoyKeys(int offset, KeysMapping &userKeys, FILE *fileOut);
  void LoadJoysticks(RString filepath);
  void LoadJoysticks(ParamEntryPtr joysticks, bool userDefined);
public:
  /// Saves active joysticks mapping into the joysticks.cfg file
  void SaveJoysticks(RString filepath);
  /// Loads schemes from config class JoystickSchemes and joysticks.cfg file in user profile folder
  void LoadJoysticks();

  // state of all connected joysticks
  AutoArray<JoystickState> _joysticksState;

  //XInput
  float xInputButtons[N_JOYSTICK_BUTTONS];
  bool xInputButtonsToDo[N_JOYSTICK_BUTTONS];
  bool xInputButtonsPressed[N_JOYSTICK_BUTTONS];
  bool xInputButtonsUp[N_JOYSTICK_BUTTONS];
  mutable unsigned char xInputMaxLevelThisFrame[N_JOYSTICK_BUTTONS]; // at which maximal level xInputButtonsToDo was asked this frame
  unsigned char xInputMaxLevelLastFrame[N_JOYSTICK_BUTTONS]; // at which maximal level xInputButtonsToDo was asked last frame
  bool xInputAutorepeat[N_JOYSTICK_BUTTONS];
  UITime xInputAutorepeatTime[N_JOYSTICK_BUTTONS];

  // True when XInput is enabled and there is also some XInput device present (XBox shortcuts are shown then)
  bool IsXInputPresent();

  //trackIR
  static const float TrackIRzfactor;
  float trackIRAxis[N_TRACKIR_AXES];
  bool trackIRAxisCleared[N_TRACKIR_AXES*2];
  static float TrackIRAxisThresholdHigh, TrackIRAxisThresholdLow;

  //freeTrack
/*
  // no axes for FreeTrack, the TrackIR axes are used
  float freeTrackAxis[N_TRACKIR_AXES];
  bool freeTrackAxisCleared[N_TRACKIR_AXES*2];
*/
  static float FreeTrackAxisThresholdHigh, FreeTrackAxisThresholdLow;

  //SpaceMouse
#if _SPACEMOUSE
  bool spaceMouseButtons[N_SPACEMOUSE_BUTTONS];
//  bool spaceMouseButtonsToDo[N_SPACEMOUSE_BUTTONS];
  float spaceMouseAxis[N_SPACEMOUSE_AXES];
//  bool spaceMouseCleared[N_SPACEMOUSE_AXES*2];
//  static float spaceMouseAxisThresholdHigh, spaceMouseAxisThresholdLow;
#endif
  // actions

  //bool fire,fireToDo;

  //float keyTurnLeft,keyTurnRight;

  //float keyMoveFastForward,keyMoveSlowForward;
  //float keyMoveForward,keyMoveBack;
  //float keyMoveUp,keyMoveDown;
  //float keyMoveLeft,keyMoveRight;

  int gameFocusLost; // how many controls prevent game to have input focus

  void SeparateComboAndKey(int dik, int &dikCombo, int &dikKey) const; //used to separate combo+key combination
  float GetKey(int dik, bool checkFocus = true, unsigned char level = 1) const;
  bool GetKeyToDo(int dik, bool reset = true, bool checkFocus = true, unsigned char level = 1);
  bool GetKeyDo(int dik, bool checkFocus = true);
  // Special Combinations are Keyboard + Mouse button (but only LCtrl as key used in UI)
  float GetSpecialCombination(int dik, bool checkFocus = true, unsigned char level = 1) const;
  bool GetSpecialCombinationToDo(int dik, bool reset = true, bool checkFocus = true, unsigned char level = 1);
  bool GetSpecialCombinationDo(int dik, bool checkFocus = true);

#if _ENABLE_CHEATS
  //@{ cheats using R-Win/R-Alt keys
  float GetCheat1(int dik) const;
  bool GetCheat1ToDo(int dik, bool reset=true);
  float GetCheat2(int dik) const;
  bool GetCheat2ToDo(int dik, bool reset=true);
  float GetCheat3(int dik) const;
  bool GetCheat3ToDo(int dik, bool reset=true);
  bool Cheats12Enabled() {return cheat12enabled;}
  void EnableCheats12(bool val) {cheat12enabled=val;}
  void ProcessVoNCheats(); // process VoN cheats
  //@}
#endif

  // Xbox cheats
  int GetCheatXToDo(CheatXType cheat, bool reset=true);
  bool GetCheatXDirectToDo(int button, bool reset=true);

  void ProcessMouse(DWORD timeDelta=1);
  // When !CanProcessMouse() some mouse variables can cause triggering some ButtonUp etc. 
  // mouse actions for ever. Calling MouseClearWhenCannotProcess() avoids this problem.
  void MouseClearWhenCannotProcess();
  float GetMouseButton(int index, bool checkFocus = true, unsigned char level = 1, bool useCombinationFilter=true) const;
  bool GetMouseButtonToDo(int index, bool reset = true, bool checkFocus = true, unsigned char level = 1, bool useCombinationFilter=true);

  /// perform mouse button processing of "level" based input eating
  bool ProcessMouseButtonLevel(int index, unsigned char level) const;
  /// perform mouse axis processing of "level" based input eating
  bool ProcessMouseAxisLevel(int index, unsigned char level) const;
  
  bool GetMouseButtonReleased(int index, bool reset = true, bool checkFocus = true, unsigned char level = 1);
  bool GetMouseButtonDo(int index, bool checkFocus = true, unsigned char level = 1);
  bool GetMouseAxisToDo(int index, bool reset, bool checkFocus, unsigned char level = 1);
  bool GetTrackIRToDo(int index, bool reset, bool checkFocus);

  float GetJoystickButton(int index, bool checkFocus = true, unsigned char level = 1) const;
  bool GetJoystickButtonToDo(int index, bool reset = true, bool checkFocus = true, unsigned char level = 1);
  bool GetJoystickButtonDo(int index, bool checkFocus = true);
  bool GetJoystickButtonAutorepeat(int index, float delay, float rate, bool reset = true, bool checkFocus = true);

  float GetXInputButton(int index, bool checkFocus = true, unsigned char level = 1) const;
  bool GetXInputButtonToDo(int index, bool reset = true, bool checkFocus = true, unsigned char level = 1);
  bool GetXInputButtonDo(int index, bool checkFocus = true);
  bool GetXInputButtonAutorepeat(int index, float delay, float rate, bool reset = true, bool checkFocus = true);

  ValueWithCurve GetActionWithCurveObject(UserAction posAction, UserAction negAction, bool checkFocus, bool exclusive) const;
  /// get action together with corresponding curve
  float GetActionWithCurve(CurveFactor factorKind, UserAction posAction, UserAction negAction, bool checkFocus = true, bool exclusive = false) const override;
  /// get action, do not apply any curve
  ValueWithCurve GetActionRaw(UserAction action, bool checkFocus, bool exclusive=false, float digitalValue=1.0f, unsigned char level = 1) const;
  /// get raw state data for the action
  void GetActionState(
    UserAction action, bool checkFocus, float &sum, float &sumAnalog, float &sumAnalogMouse, 
    float &sumXInputCustom, float &sumXInputScheme,
    bool exclusive=false, float digitalValue=1.0f, unsigned char level = 1
  ) const;

  /// similar to GetActionState, but allows to test for individual inputs
  float GetInputState(
    int key, bool checkFocus, float &sum, float &sumAnalog, float &sumAnalogMouse,
    float &sumXInputCustom, float &sumXInputScheme,
    bool exclusive=false, float digitalValue=1.0f, unsigned char level = 1
  ) const;
  /// check if given input is non-zero (raw interface to CheckAction)
  bool CheckInput(int key, bool checkFocus = true, bool exclusive=false, unsigned char level = 1) const;
  /// get action, apply curve
  float GetAction(UserAction action, bool checkFocus = true, float digitalValue=1.0f, unsigned char level = 1) const;
  /// get action exclusively (stole the input to other calls to GetAction or GetActionExclusive), apply curve
  float GetActionExclusive(UserAction action, bool checkFocus = true, float digitalValue=1.0f) const;
  /// check if starting edge was detected
  /** blockButtons blocks the input of mapped pressed button until it is released (see news:go0o36$rsf$2@new-server.localdomain) */
  bool GetActionToDo(UserAction action, bool reset = true, bool checkFocus = true, bool exclusive = false, unsigned char level = 1, bool blockButtons = false);
  /// check if starting edge was detected - raw input
  bool GetInputToDo(int key, bool reset = true, bool checkFocus = true, bool exclusive = false, unsigned char level = 1, bool blockButtons = false);

  /// check action like ToDo does, but detect state, not edge
  bool GetActionDo(UserAction action, bool checkFocus = true);

  void ChangeGameFocus(int add ) {gameFocusLost+=add;}
  void ResetGameFocus() {gameFocusLost=0;}

  Input();

  // initialize input parameters when config is loaded (pictures etc.)
  void Init();
  void SetCustomScheme(bool value);
  KeyImage GetKeyImage(int dikCode) const;

  void ForgetKeys();
  void ForgetDisplayConfigureKeys();
  void InitKeys();
  void InitMouseVariables();
  void InitJoysticksVariables();
  void ReInitDIJoysticks();  //reenumerate DI joysticks (multiple joysticks can be connected while playing)
  void LoadKeys(ParamEntryPar userCfg);
  void LoadDefaultCurves(ParamEntryPar cfg = Pars);
  void AssignDefaultJoystickKeys(int offset); //when new joystick is connected, default keys preset is used for its key mapping
  void RemoveInaccessableKeys(); //remove keys related to not connected joysticks
  void MergeJoysticksKeys(); //keys of disconnected joystick should be preserved in user config
  void ClearJoystickKeys(int offset); //clear all keys of joystick specified by ofsset
  void ResetTrackIRValues() { for(int i=0; i<N_TRACKIR_AXES; i++) trackIRAxis[i]=0; } //reset TrackIR input stored in axes (used after FreeTrack is disabled to reset values)
  void SaveKeys(ParamFile &userCfg);
  void ApplyScheme(const ParamEntry *cfg = &Pars);
  // Apply to all schemes
  void ApplyScheme(ParamEntryPar cls);
  // Apply to single scheme
  void ApplyScheme(KeysScheme &keysScheme, ParamEntryPar cls, bool yAxis);

  bool revMouse;
  bool mouseButtonsReversed;
  int  perspective;  // first person view==1, 3rd view==3
  float fov;

  /// used for mouse force feedback
  UITime mouseLastActive;

  //! Return keys mapped to given action
  KeyListEx GetActionKeys(RString action) const;
  //! Return display name of given action
  RString GetActionName(RString action) const;

  //! Check, whether this key has not been read exclusively before
  bool InputNotClosed(int key, unsigned char level) const;

  //! Blocked buttons are enabled for input just after they are released again
  void BlockButton(int key);
  void UnblockButton(int key);
  bool ButtonNotBlocked(int key) const;

#if _ENABLE_STEAM
  bool steamOverlayActive;
  void RegisterCallbacks();
  STEAM_CALLBACK_EX_MANUAL(Input, OnOverlayActivated, GameOverlayActivated_t, _callbackOverlayActivated);
  STEAM_CALLBACK_EX(Input, OnServerChangeRequested, GameServerChangeRequested_t, _callbackServerChangeRequested);
  STEAM_CALLBACK_EX(Input, OnPersonaStateChanged, PersonaStateChange_t, _callbackPersonaStateChanged);
  RString changeServer, changeServerPassword;
#endif

protected:
  //! used for GetKey, GetKeyToDo combination keys purpose
  bool CombinationFilter(int dik) const;
  //! GetKeyToDoNoCombo is called from GetKeyToDo (this is implementation before key combination were introduced)
  bool GetKeyToDoNoCombo(int dik, bool reset, bool checkFocus = true, unsigned char level = 1);
  //! Handle pressed mouse button
  void OnMouseButton(int button, bool down, DWORD time);

  //! loads info about keys from par and store it in given profile
  bool LoadProfile(ConstParamEntryPtr parClass, KeyProfile &profile);
};

extern Input GInput;

/// Joystick schemes available for mapping inside DisplayJoystickSchemes
struct JoystickKeysScheme
{
  RString name;      //joystick name
  bool obsolete;     //helper variable (scheme overwrites the one stored in joysticks.cfg)
  bool userDefined;  //scheme loaded from joysticks.cfg when true, or from config class JoystickSchemes otherwise
  KeysMapping userKeys; //key mappings

  JoystickKeysScheme() : obsolete(false), userDefined(false) {}
  void ReportContent() const; //debug purpose only
  ClassIsMovable(JoystickKeysScheme)
};

extern AutoArray<JoystickKeysScheme> GJoystickKeysSchemes;

#endif
