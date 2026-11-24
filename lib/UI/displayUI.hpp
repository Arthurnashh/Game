#ifdef _MSC_VER
#pragma once
#endif

#ifndef _DISPLAY_UI_HPP
#define _DISPLAY_UI_HPP

#if _ENABLE_NEWHEAD
# include "../heads.hpp"
#else
# include "../head.hpp"
#endif

#if _ENABLE_GAMESPY
# include "../Network/winSockImpl.hpp"
# include "../GameSpy/serverbrowsing/sb_serverbrowsing.h"
# include "../GameSpy/qr2/qr2regkeys.h"
# include "../GameSpy/common/gsAvailable.h"
# ifdef NOMINMAX
#  undef min
#  undef max
# endif
#endif


#include "uiMap.hpp"

#include <El/Network/netXboxConfig.hpp>

#if defined _XBOX && _ENABLE_MP
# if _XBOX_VER >= 200
#   include "../X360/ArmA2.spa.h"
# else
#   include "../Network/match.h"
# endif
#elif _GAMES_FOR_WINDOWS
# include "../saveGame.hpp"
# include "../X360/ArmA2.spa.h"
#endif

#include "../Network/netTransport.hpp"
#include "../Network/network.hpp"

#include "../progress.hpp"

// Voice messages are handled by Xbox Guide on Xbox 360
#if defined _XBOX && _XBOX_VER < 200 && _ENABLE_MP
# define _ENABLE_XHV 1
#else
# define _ENABLE_XHV 0
#endif

#if _ENABLE_XHV
# include <xhv.h>
#endif

/// the filename of the save based on the save type
RString GetSaveFilename(SaveGameType type);

/*!
\file
Interface file for particular displays.
*/

//! Display for difficulty selection (Xbox)
class DisplaySelectDifficulty : public Display
{
protected:
	//! next dialog id
	int _iddNext;

	//! difficulties list
	InitPtr<CListBoxContainer> _list;
	
	//! difficulty description
	InitPtr<CHTMLContainer> _desc;

public:
	//! constructor
	/*!
		\param parent parent display
	*/
	DisplaySelectDifficulty(ControlsContainer *parent, int iddNext,RString rsc, bool enableSimulation);
	Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
	
	int GetIDDNext() const {return _iddNext;}
	int GetDifficulty() const;

	void OnLBSelChanged(IControl *ctrl, int curSel);
	void OnLBDblClick(int idc, int curSel);

protected:
	void OnChangedDifficulty();
#if _GAMES_FOR_WINDOWS || (defined _XBOX && _XBOX_VER >= 200)
  /// handle storage device removal
  virtual void OnStorageDeviceRemoved();
#endif
};

struct ListItem
{
  RString name;
  RString displayName;
};
TypeIsMovableZeroed(ListItem)

class DisplayProfileController : public Display
{
protected:
  InitPtr<CListBoxContainer> _vehicle;
  InitPtr<CListBoxContainer> _types;
  InitPtr<CListBoxContainer> _sensitivity;
  InitPtr<ITextContainer> _scheme;

  AutoArray<ListItem> _schemeList;
  AutoArray<ListItem> _typeList;

  InitPtr<CStructuredText> _keyA;
  InitPtr<CStructuredText> _keyB;

  ControllerScheme _current;
  bool _yAxis;
  RString _type;

  int _lastTypes;

public:
  //! constructor
  /*!
  \param parent parent display
  */
  DisplayProfileController(ControlsContainer *parent, bool enableSimulation, const ControllerScheme &scheme);
  Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
  void OnButtonClicked(int idc);
  void OnLBSelChanged(IControl *ctrl, int curSel);
  void OnSimulate(EntityAI *vehicle);

  const ControllerScheme &GetScheme() const {return _current;}

protected:
  void SetYAxis();

  void Init();
  void OnSchemeChanged();
  void OnVehicleChanged();
  void OnTypeChanged();
  void UpdateTexts();

  int FindScheme(RString scheme) const;
  void UpdateTypes();
  void UpdateVehicles(RString name);

  // update the buttons texts based on the current context
  void UpdateButtons();
};

#if !defined(_XBOX) && defined(_WIN32)
//! Control for user controls configuration
class CKeys
{
protected:

	//! action to keys assignment
	AutoArray<int> _keys[UAN];
	//! collisions in definitions (if one key is assigned to several action)
	AutoArray<bool> _collisions[UAN];

	//! selected column
	int _mode;
	//! ignored user input (do not process key used for enter into column)
	int _ignoredKey;

public:
	//! constructor
	/*!
		Used when control is created by resource template.
		\param parent control container by which this control is owned
		\param idc id of control
		\param cls resource template
	*/
	CKeys();

	//! returns actual column
	int GetMode() const {return _mode;}
  //! sets actual column
  void SetMode(int mod) {if (mod>=-1 && mod<=0) _mode=mod; }
	//! avoid usage of ignored key
	void CheckIgnoredKey();

	
	//! returns whole key assignment
	const AutoArray<int> &GetKeys(int action) const {return _keys[action];}
	const AutoArray<int> *GetKeys() const {return _keys;}
	//! sets whole key assignment
	void SetKeys(int action, const AutoArray<int> &keys);
	//! removes the last assigned key from action
	void RemoveKey(int action);
  //! removes the specified key from action if such
  void RemoveKey(int action, int key);
	//! adds new key to action
	void AddKey(int action, int key);

protected:
	//! updates collision array for given key
	void CheckCollisions(int key);
};

class C2DKeys : public CListBox, public CKeys
{
protected:
  bool _doNotOpenConfigureAction;
  const AutoArray<int> *_actions;
public:
  PackedColor _collisionColor;
  PackedColor _disabledKeyColor;

  float _mainCollumW;
  float _secndCollumW;
  //! constructor
  /*!
  Used when control is created by resource template.
  \param parent control container by which this control is owned
  \param idc id of control
  \param cls resource template
  */
  C2DKeys(ControlsContainer *parent, int idc, ParamEntryPar cls);

  bool OnKeyDown(int dikCode);
  bool OnKeyUp(int dikCode) {return true;}
  bool OnChar(unsigned nChar, unsigned nRepCnt, unsigned nFlags) {return true;}
  void OnLButtonDown(float x, float y);

  void DrawItem
  (
    UIViewport *vp, float alpha, int i, bool selected, float top,
    const Rect2DFloat &rect, float widthDiff
  ) override;
  bool GetDoNotConfigureAction() {return _doNotOpenConfigureAction;}
  void SetDoNotConfigureAction(bool val) {_doNotOpenConfigureAction=val;}
  bool IsCollision(int i, int dikCode);
  RString GetCollisionName(int curSel, int dikCode);
  int GetFirstCollisionAction(int curSel, int dikCode);
  void SetActionGroup(const AutoArray<int> *actions) { _actions = actions; }
  int GetAction(int curSel) { return (_actions ? (*_actions)[curSel] : 0); }
  int GetActionIx(int action);
};

#include <Es/Containers/listBidir.hpp>
class DisplayConfigureAction : public Display
{
protected:
  InitPtr<CListBoxContainer> _keys;
  InitPtr<CListBoxContainer> _special;
  PackedColor _noCollisionCol, _collisionColor, _disabledKeyColor;
  C2DKeys *_allKeys;
  int _curSel;
  AutoArray<int> _oldkeys;
  class ActiveKeyItem : public RefCount, public TLinkBidirRef
  {
  public:
    int dik;
    ActiveKeyItem(int key) : dik(key) {}
  };
  TListBidirRef<ActiveKeyItem> _activeKeyList;

  // when dialog exists as Exit(IDC_CONFIGURE_ACTION_NEXT), DisplayConfigure dialog should create new DisplayConfigureAction child using _nextAction
  int _nextAction;

public:
  //! constructor
  /*!
  \param parent parent display
  \param enableSimulation enable game simulation
  - true if called form main menu
  - false in case of in game invocation
  */
  DisplayConfigureAction(ControlsContainer *parent, C2DKeys *keys, int curSel, bool enableSimulation, bool curSelIsAction=false);
#if _ENABLE_CHEATS
  ~DisplayConfigureAction() {GInput.EnableCheats12(true);}
#endif

  Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
  virtual bool OnLBDragging(IControl *ctrl, float x, float y);
  virtual void OnLBDblClick(int idc, int curSel);
  virtual bool OnLBDrop(IControl *ctrl, float x, float y);
  virtual void OnButtonClicked(int idc);
  virtual bool OnKeyDown(int dikCode);
  virtual bool OnKeyUp(int dikCode);
  virtual void OnSimulate(EntityAI *vehicle);

  int  AddKey(int value);
  int AddDikCode(int dikCode);
  bool RemoveKey(int value);
  void RestoreBackupKeys();
  int GetAction(int curSel) { return _allKeys->GetAction(curSel); }
  void ForgetKeys();
  int NextAction() const { return _nextAction; }
};

//! User control configuration display
class DisplayConfigure : public Display
{
protected:
  InitPtr<CListBoxContainer> _mouseReversed;
	//@{
	//! original value (restored when Cancel performed)
	bool _oldJoystickEnabled;
	bool _oldMouseButtonsReversed;
	float _oldMouseSensitivityX;
	float _oldMouseSensitivityY;
  int _oldMouseFiltering;
	//@}

public:
	//! constructor
	/*!
		\param parent parent display
		\param enableSimulation enable game simulation
		- true if called form main menu
		- false in case of in game invocation
	*/
	DisplayConfigure(ControlsContainer *parent, bool enableSimulation);

	Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
  bool OnKeyDown(int dikCode);
	void OnSimulate(EntityAI *vehicle);
	void OnButtonClicked(int idc);
	void OnSliderPosChanged(IControl *ctrl, float pos);
  void OnLBSelChanged(IControl *ctrl, int curSel);
	void Destroy();
  void OnChildDestroyed(int idd, int exit);
  void UpdateControlsPages();
  //void UpdateKeys();
};

//! User control configuration display - Action Keys Assignments
class DisplayConfigureAssignments : public Display
{
protected:
	//! configuration listbox
  C2DKeys *_keys;
  //! configuration page control (combo)
  InitPtr<CListBoxContainer> _controlsPages;
  int _currentActionGroup ;

public:
	/*!
		\param parent parent display
		\param enableSimulation enable game simulation
		- true if called form main menu
		- false in case of in game invocation
	*/
	DisplayConfigureAssignments(ControlsContainer *parent, bool enableSimulation);

	Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
	void OnButtonClicked(int idc);
  void OnLBSelChanged(IControl *ctrl, int curSel);
	void Destroy();
  void OnChildDestroyed(int idd, int exit);
  void UpdateControlsPages();
  void UpdateKeys();

  // Create ConfigureActionChild (FIX: do not call it from ConfigureActionChild as a reaction to Prev/Next buttons)
  void ChangeConfigureActionChild(int curSel);
};

class DisplayControlSchemes : public Display
{
protected:
  // list with all default profiles
  InitPtr<CListBoxContainer> _profileList; 
  // out array of key definition
  C2DKeys *_outKeys;
public:
  DisplayControlSchemes(ControlsContainer *parent, C2DKeys *keys);
  Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
  void Destroy();
};

class DisplayConfigureControllers : public Display
{
protected:
  // List of all currently connected joysticks
  bool _xInputEnabled;
  InitPtr<CListBoxContainer> _joysticks;
  PackedColor _joysticksDisabledColor;
  InitPtr<CListBoxContainer> _xInputControllers;
  PackedColor _xInputDisabledColor;

  //@{
  //! control variables of the selected joystick
  enum {AListControllers, AListXInput} _activeList;
  //@}

public:
  enum {CT_TrackIR=-1, CT_FreeTrack=-2};
  /*!
  \param parent parent display
  \param enableSimulation enable game simulation
  - true if called form main menu
  - false in case of in game invocation
  */
  DisplayConfigureControllers(ControlsContainer *parent, bool enableSimulation);

  Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
  virtual void OnButtonClicked(int idc);
  virtual void OnLBSelChanged(IControl *ctrl, int curSel);
  virtual void OnChildDestroyed(int idd, int exit);
  void RefreshButtons();
  void UpdateJoysticksList();
  void UpdateJoyColor(int ix, bool val);
};

class DisplayCustomizeController : public Display
{
protected:
  int      _controllerIx;  // CT_TrackIR or joyIx
  enum {CDANone, CDAClearKeys, CDADefaultKeys} _performActionOnOK; // unMap keys or set them to default after OK click
  RefArray<Control> _sensitivityCtrl;
  RefArray<Control> _deadZoneCtrl;
  RefArray<Control> _steepnessCtrl;
  int _axisIx[N_JOYSTICK_AXES]; // index of given enumerated axis as known in GJoystickDevices, InputState etc.
  bool _showAdvanced;
  float _vSpacing; // the space between sliders
  Ref<Texture> _activeAxisIcon;
  bool _updateDeadZones;
  DWORD _dzNextUpdate;

  FindArray<int> _allKeys[UAN];
  
  float GetSensitivity(CXSlider *slider);
  void SetDefaultSensitivity();
  void SetDefaultDeadZone();
  float GetDeadZone(CXSlider *slider);
  void SetDefaultSteepness();


  void ClearControllerKeys();
  void SetDefaultKeys();

  Control *AddControl(CControlsGroup *grp, int ctrlIx, int axisIx, float &top, RString name);
  void OnChanged(IControl *ctrl);

public:
  /*!
  \param parent parent display
  \param enableSimulation enable game simulation
  - true if called form main menu
  - false in case of in game invocation
  */
  DisplayCustomizeController(ControlsContainer *parent, int controllerIx, bool enableSimulation);

  void SetKeysByScheme(const JoystickKeysScheme &scheme);

  virtual void OnButtonClicked(int idc);
  virtual void OnSliderPosChanged(IControl *ctrl, float pos) {OnChanged(ctrl);}
  virtual void OnLBSelChanged(IControl *ctrl, int curSel);
  virtual void OnChildDestroyed(int idd, int exit);

  virtual void OnSimulate(EntityAI *vehicle);
};

/// Dialog allow to map keys from available schemes to selected joystick
class DisplayJoystickSchemes : public Display
{
protected:
  // List of all joystick schemes available to map to selected joystick
  InitPtr<CListBoxContainer> _schemes;
  FindArray<int> *_allKeys; // to set the joystick keys using given scheme

public:
  /*!
  \param parent parent display
  \param enableSimulation enable game simulation
  - true if called form main menu
  - false in case of in game invocation
  */
  DisplayJoystickSchemes(DisplayCustomizeController *parent, FindArray<int> *keys, bool enableSimulation, RString joyName);

  Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
  virtual void OnButtonClicked(int idc);

private:
  void UpdateSchemesList();
};

#if _ENABLE_EDITOR

//! Select island display (notebook)
class DisplaySelectIsland : public Display
{
protected:
	//! wanted exit code (exit is performed after close animation is finished)
	int _exitWhenClose;

public:
	//! constructor
	/*!
		\param parent parent display
	*/
	DisplaySelectIsland(ControlsContainer *parent)
		: Display(parent)
	{
		Load("RscDisplaySelectIsland");
		_exitWhenClose = -1;
	}

	Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
	void OnLBDblClick(int idc, int curSel);
	void OnButtonClicked(int idc);
	void OnChildDestroyed(int idd, int exit);
	void OnCtrlClosed(int idc);
};

#endif // _ENABLE_EDITOR

//! All missions (for designers) display
class DisplayCustomArcade : public Display
{
public:
	//! constructor
	/*!
		\param parent parent display
	*/
	DisplayCustomArcade(ControlsContainer *parent)
		: Display(parent)
	{
		Load("RscDisplayCustomArcade");
		InsertGames();
		ShowButtons();
	}

	Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
	void OnButtonClicked(int idc);
	void OnTreeSelChanged(IControl *ctrl);
	void OnChildDestroyed(int idd, int exit);
  void OnSimulate(EntityAI *vehicle);

	bool CanDestroy();

// implementation
protected:
	//! creates missions tree
	void InsertGames();
	//! updates buttons when selected item in tree changes
	void ShowButtons();
};
#endif


#if _XBOX_SECURE
class DisplayTestNetworkConditions : public Display
{
protected:
#if _XBOX_VER >= 200
  Buffer<BYTE> _buffer;
  XOVERLAPPED _operation;
#else
  XONLINETASK_HANDLE _task;
#endif
  XNQOS *_qosResults;

  DWORD _timeout;

  InitPtr<ITextContainer> _text;
  Ref<CAnimatedTexture> _time;

public:
  DisplayTestNetworkConditions(ControlsContainer *parent, XONLINE_FRIEND &f);
  ~DisplayTestNetworkConditions();

  Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
  void OnSimulate(EntityAI *vehicle);
  void OnButtonClicked(int idc);
};
#endif

//! Multiplayer type display (for Xbox)
class DisplayMultiplayerType : public Display
{
protected:
	InitPtr<ITextContainer> _download;
	InitPtr<CHTMLContainer> _info;

#if defined _XBOX && _XBOX_VER >= 200
  /// index of last button clicked
  int _lastButtonClicked;
  /// if we should test storage device in OnButtonClicked
  bool _testStorageDevice;
  /// if we should test storage device in OnInvite
  bool _testStorageOnInvite;
#endif


#if defined _XBOX && _XBOX_SECURE // downloadable content only on Xbox
# if _XBOX_VER >= 200
    /// Operation checking for the existing downloadable content
    Ref<OverlappedOperation> _operation;
    /// Result of _operation
    XOFFERING_CONTENTAVAILABLE_RESULT _operationResults;
# else
    bool _request;
    bool _invitation;
    bool _newContent;

	  XONLINETASK_HANDLE _task;

    Ref<CStructuredText> _friends;
# endif
#endif

public:
	//! constructor
	/*!
		\param parent parent display
	*/
	DisplayMultiplayerType(ControlsContainer *parent);
#ifdef _XBOX
  DisplayMultiplayerType(ControlsContainer *parent, const XONLINE_FRIEND &f);
#endif
	~DisplayMultiplayerType();

	Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
	void OnButtonClicked(int idc);
	void OnChildDestroyed(int idd, int exit);
	void OnSimulate(EntityAI *vehicle);

protected:
  void Init();
#if _XBOX_SECURE && _XBOX_VER < 200 && _ENABLE_MP
	void JoinToFriend(const XONLINE_FRIEND &f);
  void UpdateAppearOffline();
#endif

#if _GAMES_FOR_WINDOWS || (defined _XBOX && _XBOX_VER >= 200)
  /// handle invitation
  virtual void OnInvited();
#endif
};

/// Xbox Live main screen
class DisplayLive : public Display
{
protected:
  SessionType _sessionType;

public:
  DisplayLive(ControlsContainer *parent, SessionType sessionType);

  void OnButtonClicked(int idc);
  void OnChildDestroyed(int idd, int exit);
};

/// Xbox Dedicated server settings (max. number of players, slots reserverd for friends)
class DisplayDedicatedServerSettings : public Display
{
protected:
  InitPtr<CSliderContainer> _totalSlots;
  InitPtr<CSliderContainer> _privateSlots;

  int _defaultMaxPlayers;

  int _bandwidth;
  int _slots;

public:
  DisplayDedicatedServerSettings(ControlsContainer *parent);

  Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
  void OnButtonClicked(int idc);
  void OnChildDestroyed(int idd, int exit);
  void OnSimulate(EntityAI *vehicle);

  int GetBandwidth() const {return _bandwidth;}
  int GetPrivateSlots() const {return _slots;}

protected:
  void LaunchDS();
};

enum SortColumn
{
	SCServer,
  SCGameType,
	SCMission,
	SCState,
	SCPlayers,
	SCPing,
  SCFriends,
  SCTimeOfDay,
  SCHive,
  SCMode
};

//! Control for displaying info about sessions
class CSessionsContainer
{
friend class DisplayMultiplayer;
friend class DisplaySystemLink;
friend class DisplayOptiMatch;

protected:
  //! host session
  SessionInfo _host;
	//! session descriptions
	AutoArray<SessionInfo> _sessions;
  //! indices of valid sessions (sorted)
  AutoArray<int> _indices;
	//! lock picture - shown when session is password protected
	Ref<Texture> _password;
	//! picture shown when session has incompatible version
	Ref<Texture> _version;
  //! picture shown when session has some problems with addons (missing signature files, bad mods, ...)
  Ref<Texture> _addons;
  //! picture shown when session has possible problem with addons (missing mods, very improbable to connect)
  Ref<Texture> _mods;
  //! picture shown when session is locked
  Ref<Texture> _lockText;
	//! no picture
	Ref<Texture> _none;

  /// ping colors
  PackedColor _colorPingUnknown;
  PackedColor _colorPingGood;
  PackedColor _colorPingPoor;
  PackedColor _colorPingBad;

public:
	//! constructor
	CSessionsContainer(ParamEntryPar cls);
	virtual ~CSessionsContainer() {}

	void Sort(SortColumn column, bool ascending, bool keepSelection);

#define DISABLE_HOST_SESSION 1
#if DISABLE_HOST_SESSION
  int NSessions() const {return _indices.Size();}
  SessionInfo &GetSession(int i) {return _sessions[_indices[i]];}
  const SessionInfo &GetSession(int i) const {return _sessions[_indices[i]];}
#else
  int NSessions() const {return _indices.Size() + 1;}
  const SessionInfo &GetSession(int i) const {return i >= _indices.Size() ? _host : _sessions[_indices[i]];}
#endif
  int NSessionsNoHost() const {return _indices.Size();}

  RString GetSessionID(int i) const;
	virtual int SelectedSession() const = 0;
	virtual void SelectSession(int sel) = 0;

  /// delete the session and update indices
  void DeleteSession(int index);

  /// return the icon related to given session
  Texture *GetSessionIcon(int index) const;
};

class C2DSessions : public CListBox, public CSessionsContainer
{
private:
  Ref<Texture> _star;

  static const int _columns = 8;
  float _colWidths[_columns];

public:
	C2DSessions(ControlsContainer *parent, int idc, ParamEntryPar cls);

	int GetSize() const {return NSessions();}
  // multiple selection is not allowed
  bool IsSelected(int row, int style) const {return row == GetCurSel();}
	RString GetData(int i) const {return GetSessionID(i);}

	int SelectedSession() const {return GetCurSel();}
	void SelectSession(int sel) {SetCurSel(sel);}

	void DrawItem
	(
		UIViewport *vp, float alpha, int i, bool selected, float top,
		const Rect2DFloat &rect, float widthDiff
	) override; 
};

#if _ENABLE_GAMESPY
#define BROWSING_SOURCE_ENUM(type, prefix, XX) \
	XX(type, prefix, LAN) \
	XX(type, prefix, Internet) \
	XX(type, prefix, Remote)
#elif _ENABLE_STEAM && defined(_WIN64)
#define BROWSING_SOURCE_ENUM(type, prefix, XX) \
	XX(type, prefix, LAN) \
	XX(type, prefix, Remote) \
	XX(type, prefix, Internet)
#else
#define BROWSING_SOURCE_ENUM(type, prefix, XX) \
	XX(type, prefix, LAN) \
	XX(type, prefix, Remote)
#endif

#ifndef DECL_ENUM_BROWSING_SOURCE
#define DECL_ENUM_BROWSING_SOURCE
DECL_ENUM(BrowsingSource)
#endif
DECLARE_ENUM(BrowsingSource, BS, BROWSING_SOURCE_ENUM)

struct SessionFilter : SerializeClass
{
	RString serverName;
  RString missionType;
	RString missionName;
	int maxPing;
	int minPlayers;
	int maxPlayers;
	bool fullServers;
	bool passwordedServers;
  bool expansionsServers;
  enum BEFilter {BEFLT_NONE, BEFLT_REQUIRED, BEFLT_WITHOUT_BE, BEFLT_COUNT} battleyeRequired;

  SessionFilter()
  {
    maxPing = 100;
    minPlayers = 0;
    maxPlayers = 0;
    fullServers = true;
    passwordedServers = true;
    expansionsServers = true;
    battleyeRequired = BEFLT_NONE;
  }
  LSError Serialize(ParamArchive &ar);
};

//! Base multiplayer display
/*!
	Shows available sessions on LAN or given remote computer.
	Enables join into running session or create new session.
*/
enum MPSessionType {MPTMultiplayer, MPTHostMission};
enum MPTabs {
  MPTabInternet, MPTabFavorite, MPTabHistory, MPTabFriends, MPTabLAN, MPTabRemote, MP_TABS_COUNT
};

class SteamServerListBridge;
class gameserveritem_t;

class DisplayMultiplayer : public Display
{
protected:
	//! IP address of computer for which sessions are displayed (empty for LAN)
	RString _ipAddress;
	//! port of computer for which sessions are displayed (on local network)
	int _portLocal;
	//! port of computer for which sessions are displayed (on remote server)
	int _portRemote;
	//! wanted exit code (exit is performed after close animation is finished)
	int _exitWhenClose;
	//! sessions info listbox
	CSessionsContainer *_sessions;

#if _ENABLE_GAMESPY
	//! GameSpy server browser
  ServerBrowser _serverBrowser;
  /// Server browser update callback
  SRef<INetworkUpdateCallback> _serverBrowserUpdate;
  /// lock to disable list update during additional session info query
  enum {Stopped, StageRunning, StageDone, Completed} _listUpdateRunning;
  int _listUpdateStage,_listUpdateMaxStage;
  /// when the update of the current server will be called
  SBServer _selected;

  //@{ client assumed location
  int _latitude,_longitude;
  //@}

#endif

#if _ENABLE_STEAM && defined(_WIN64)
  friend class SteamServerListBridge;
  SteamServerListBridge *_steamServerList;
  void AddSteamServer(const gameserveritem_t &server);
  void FinishSteamServerList();
#endif


  MPTabs _activeTab;
	//! which sessions are listed
	BrowsingSource _source;

	//! sort by
	SortColumn _sort;
	//! sort order
	bool _ascending;

  //! force update of sessions
  bool _refresh;

  /// was some session already selected by the user after Refresh started
  bool _sessionSelected;

  //! ascending sort picture name
	RString _sortUp; 
	//! descending sort picture name
	RString _sortDown;

	//! filter for sessions
	SessionFilter _filter;

  /// filter icons (passworded servers)
  Ref<Texture> _showPasswored, _hidePassworded;
  /// filter icons (BattlEye servers)
  Ref<Texture> _showBattlEye, _showNoBattlEye, _hideBattlEye;
  /// filter icons (full servers)
  Ref<Texture> _showFull, _hideFull;
  /// filter icons (servers containing unknown expansions (mods))
  Ref<Texture> _showExpansions, _hideExpansions;

  /// Detailed info colors
  PackedColor _colorPingUnknown;
  PackedColor _colorPingGood;
  PackedColor _colorPingPoor;
  PackedColor _colorPingBad;

  PackedColor _colorVersionGood;
  PackedColor _colorVersionBad;

public:
	//! constructor
	/*!
		\param parent parent display
	*/
	DisplayMultiplayer(ControlsContainer *parent);
	//! destructor
	~DisplayMultiplayer();

	Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls) override;
	void OnSimulate(EntityAI *vehicle) override;
  void OnSimulateBackground(EntityAI *vehicle) override;
	void OnButtonClicked(int idc) override;
  void OnLBSelChanged(IControl *ctrl, int curSel) override;
	void OnLBDblClick(int idc, int curSel) override;
	void OnCtrlClosed(int idc) override;
	void OnChildDestroyed(int idd, int exit) override;

  void OnSimulateBasic();
#if _ENABLE_GAMESPY
  /// Called from the GameSpy SDK when server browser was updated
  void OnSBUpdated(ServerBrowser serverBrowser, SBCallbackReason reason, SBServer server);
#endif

  // get selected session
  const SessionInfo * CurrentSession() const;
  // friends array list of the selected server
  void Friends(AutoArray<RString> &friends);
  // to get passworded icon texture
  Texture *GetCurrentSessionIcon() const;

protected:
  //! update server list update progress
  void SetProgress(float progress);
	//! updates display (buttons, session list) when IP address changed
	void UpdateAddress(RString ipAddress, int port);
	//! updates list of session (called in each simulation step)
	void UpdateSessions();
#if _ENABLE_GAMESPY
	//! updates list of session on internet (using GameSpy browser)
	void UpdateServerList();

  //! perform one "stage" of update
  bool StageUpdateServerList(int stage);
  //! add / update a server
  void UpdateServer(SBServer server, bool add);
  //! remove a server
  void RemoveServer(SBServer server);

#endif
#if _ENABLE_STEAM && defined(_WIN64)
  //! updates public sessions from the Steam game-server master list
  void UpdateSteamServerList();
#endif
  //! update display when other session was selected
  void OnSessionChanged(bool changed);
	//! returns current port
	int GetPort();
	//! sets current port
	void SetPort(int port);
	//! set session source
	void SetSource(BrowsingSource source, bool save);
	//! update sort icons
	void UpdateIcons();
	//! update filter titles
	void UpdateFilter();

  //! set favorite member and changes the text inside button IDC_MULTI_FAVORITE 
  void UpdateSessionFavorite(const SessionInfo *session);

	//! load options for display from user profile
	void LoadParams();
	//! save options for display to user profile
	void SaveParams();

  //! show/hide filter icons (these should not be shown when LAN servers are listed)
  void ShowFilterIcons(bool show);

  //! connect to session identified by guid
  void JoinSession(SessionType sessionType, RString guid, RString password, const SessionInfo *session);
  //! create a new session
  void CreateSession(SessionType sessionType, RString name, RString password, int maxPlayers, bool isPrivate, int port, MPSessionType mpType = MPTMultiplayer, ConstParamEntryPtr *cfg = NULL);

#if _USE_BATTL_EYE_SERVER
  //! creates messages box for user to confirm the BE licence
  void CreateBELicenceMsgBox();
#endif //_USE_BATTL_EYE_SERVER

  //! Set active tab (internet, LAN, remote, friends, ...)
  void SetTab(MPTabs tab=MP_TABS_COUNT);
};

class DisplayMultiplayerDetails : public Display
{
  DWORD _lastFriendsUpdate;
public:
	//! constructor
	/*!
		\param parent parent display
	*/
  DisplayMultiplayerDetails(ControlsContainer *parent);
  ~DisplayMultiplayerDetails();

	Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
	void OnSimulate(EntityAI *vehicle);
	void OnButtonClicked(int idc);
  void RefreshDetails();

  void RefreshVariables();
};

//! Host settings display (basic parameters of the session)
class DisplayHostSettings : public Display
{
protected:
  InitPtr<ITextContainer> _name;
  InitPtr<ITextContainer> _password;
  InitPtr<ITextContainer> _maxPlayers;
  InitPtr<CListBoxContainer> _private;
  bool _isPrivate;
  InitPtr<ITextContainer> _port;

public:
  //! constructor
  /*!
  \param parent parent display
  */
  DisplayHostSettings(ControlsContainer *parent);

  RString GetName() const {return _name ? _name->GetText() : RString();}
  RString GetPassword() const {return _password ? _password->GetText() : RString();}
  int GetMaxPlayers() const;
  bool IsPrivate() const {return _isPrivate;}
  int GetPort() const;

  Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
  void OnLBSelChanged(IControl *ctrl, int curSel);
};

#ifndef _XBOX
//! Password display (enter password for multiplayer session)
class DisplayPassword : public Display
{
protected:
	//! session id
	RString _guid;
  RString _serverName;

public:
	//! constructor
	/*!
		\param parent parent display
		\param guid session id
	*/
	DisplayPassword(ControlsContainer *parent, RString guid, RString name=RString(""))
		: Display(parent)
	{
		_enableSimulation = false;
		_guid = guid;
    _serverName = name;
		Load("RscDisplayPassword");
	}
	Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
  RString GetGUID() const {return _guid;}
  RString GetPassword() const;
  RString GetServerName() const { return _serverName; }
};

//! Port display (enter port for multiplayer session)
class DisplayPort : public Display
{
protected:
	//! actual password
	int _port;

public:
	//! constructor
	/*!
		\param parent parent display
		\password current password
	*/
	DisplayPort(ControlsContainer *parent, int port)
		: Display(parent)
	{
		_enableSimulation = false;
		_port = port;
		Load("RscDisplayPort");
	}
	Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
};
#endif

#if defined _XBOX && _XBOX_VER < 200

// On Xbox 360, profiles are handled by Xbox Guide

//! Xbox select Xbox Live account display
class DisplayProfileLive : public Display
{
protected:
  InitPtr<CListBoxContainer> _accounts;
#if defined _XBOX && _ENABLE_MP
  XONLINE_USER _users[XONLINE_MAX_STORED_ONLINE_USERS];
  DWORD _usersCount;
#endif
  bool _mainMenu;

public:
  //! constructor
  /*!
  \param parent parent display
  */
  DisplayProfileLive(ControlsContainer *parent, bool enableSimulation, RString account, bool mainMenu = false);
  Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);

  void OnButtonClicked(int idc);
  void OnSimulate(EntityAI *vehicle);
  void OnChildDestroyed(int idd, int exit);

#ifdef _XBOX
  RString GetAccount() const;
protected:
  void UpdateAccounts(RString account);
  void SignIn(const XONLINE_USER &user);
#endif
};

#endif // defined _XBOX && _XBOX_VER < 200

//! Xbox system link display
/*!
	Shows available sessions on LAN.
	Enables join into running session or create new session.
*/
class DisplaySystemLink : public Display
{
protected:
	//! sessions info listbox
	CSessionsContainer *_sessions;
	//! process enumeration by NetworkManager
	bool _enumeration;

#if _XBOX_SECURE
  ULONGLONG _rating;
#endif

public:
	//! constructor
	/*!
		\param parent parent display
	*/
	DisplaySystemLink(ControlsContainer *parent, bool enumeration = true);
	//! destructor
	~DisplaySystemLink();

	Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
	void OnSimulate(EntityAI *vehicle);
	void OnButtonClicked(int idc);
	void OnChildDestroyed(int idd, int exit);

  virtual SessionType GetSessionType() const {return STSystemLink;}

protected:
	//! updates list of session (called in each simulation step)
	void UpdateSessions();
	//! update session details when selected session changed
	void OnSessionChanged();

  void HostSession();

	//! suspend enumeration of sessions
	virtual void SuspendEnum() {}
	//! resume enumeration of sessions
	virtual void ResumeEnum() {}
};

//! Xbox OptiMatch Filter display
class DisplayOptiMatchFilter : public Display
{
protected:
	InitPtr<CListBoxContainer> _gameType;
	InitPtr<CListBoxContainer> _minPlayers;
	InitPtr<CListBoxContainer> _maxPlayers;
  InitPtr<CListBoxContainer> _language;
  InitPtr<CListBoxContainer> _difficulty;

public:
	DisplayOptiMatchFilter(ControlsContainer *parent);

	Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);

	RString GetGameType() const;
	int GetMinPlayers() const;
	int GetMaxPlayers() const;
  int GetLanguage() const;
  int GetDifficulty() const;
  int GetMaxDifficulty() const;
};

/*
enum EnumSessionsPhase
{
	ESPQuery,
	ESPQoS,
	ESPDone
};
*/

#if _GAMES_FOR_WINDOWS || (defined _XBOX && _XBOX_VER >= 200)
// current state of the query
enum XLiveQueryState
{
  QSNone,
  QSInit,
  QSSize,
  QSQuery,
  QSSucceeded,
  QSProbe,
  QSProbeSucceeded,
  QSFailed
};

// Xbox Live query encapsulation
class XLiveQuery
{
private:
  static const DWORD _resultsCount = 25;

  DWORD _queryIndex;
  DWORD _userIndex;

  // XUSER_PROPERTY _properties[0];
  XUSER_CONTEXT _contexts[2];

  XLiveQueryState _state;

  DWORD _resultsSize;
  Buffer<BYTE> _results;

  /// the state of the current operation
  XOVERLAPPED _overlapped;

  /// results of QoS
  XNQOS *_qos;

public:
  XLiveQuery();
  ~XLiveQuery();

  /// return the current state
  XLiveQueryState GetState() const {return _state;}

  /// initialize the query
  bool Init(SessionType sessionType);
  /// (re)start the query
  bool Start();
  /// interrupt the query
  void Cancel();

  /// make the query working
  XLiveQueryState Process();
  /// read the query results
  bool Read(AutoArray<SessionInfo> &sessions);

  /// (re)start the probing
  bool Probe();
  /// update the info by probing results
  bool Update(AutoArray<SessionInfo> &sessions);
};
#endif

#if _GAMES_FOR_WINDOWS || (defined _XBOX && _XBOX_VER >= 200)
typedef int QueryValueType;
static QueryValueType QueryValueNone = -1;
#elif defined _XBOX
typedef ULONGLONG QueryValueType;
static QueryValueType QueryValueNone = X_MATCH_NULL_INTEGER;
#else
typedef int QueryValueType;
static QueryValueType QueryValueNone = -1;
# define XC_LANGUAGE_ENGLISH    1
# define XC_LANGUAGE_GERMAN     3
# define XC_LANGUAGE_FRENCH     4
# define XC_LANGUAGE_ITALIAN    6
#endif

//! Xbox OptiMatch display
/*!
	Shows sessions available through Xbox Live.
	Enables join into running session or create new session.
*/
class DisplayOptiMatch : public DisplaySystemLink
{
protected:
	typedef DisplaySystemLink base;

  int _suspended;

  QueryValueType _gameType;
  QueryValueType _minPlayers;
  QueryValueType _maxPlayers;
  QueryValueType _language;
  QueryValueType _difficulty;
  QueryValueType _maxDifficulty;
  QueryValueType _prefferedDifficulty;

#if _GAMES_FOR_WINDOWS || (defined _XBOX && _XBOX_VER >= 200)
  XLiveQuery _query;
#elif _XBOX_SECURE && _ENABLE_MP
  COptiMatchQuery _query;
#endif

  SessionType _sessionType;
  UITime _nextQosTime;

public:
	//! constructor
	/*!
		\param parent parent display
	*/
	DisplayOptiMatch(ControlsContainer *parent, SessionType sessionType, RString gameType, int minPlayers, int maxPlayers, int language, int difficulty, int maxDifficulty);
	//! destructor
	~DisplayOptiMatch();

  void OnButtonClicked(int idc);
	void OnLBSelChanged(IControl *ctrl, int curSel);
	void OnSimulate(EntityAI *vehicle);

  virtual SessionType GetSessionType() const {return _sessionType;}

protected:
	void SuspendEnum();
	void ResumeEnum();

  void UpdateButtons();
};

//! Xbox Quick Match display
/*!
	Offers sessions available through Xbox Live.
	Enables join into running session or create new session.
*/
class DisplayQuickMatch : public Display
{
protected:
	//! session descriptions
	AutoArray<SessionInfo> _sessions;

	int _selected;

#if _GAMES_FOR_WINDOWS || (defined _XBOX && _XBOX_VER >= 200)
  // Ranking is handled automatically for Ranked Matches
  XLiveQuery _query;
#elif _XBOX_SECURE && _ENABLE_MP
  ULONGLONG _rating;
  COptiMatchQuery _query;
#endif

  SessionType _sessionType;
  UITime _nextQosTime;

public:
	//! constructor
	/*!
		\param parent parent display
	*/
	DisplayQuickMatch(ControlsContainer *parent, SessionType sessionType);
  ~DisplayQuickMatch();

	void OnButtonClicked(int idc);
	void OnChildDestroyed(int idd, int exit);
	void OnSimulate(EntityAI *vehicle);

protected:
	void InitSessions();
	void UpdateSession();
};

#if _XBOX_SECURE && _ENABLE_MP

extern XONLINE_FRIEND GSelectedFriend;

// Friends, feedback and invitations are handled by Xbox Guide on Xbox 360
#if _XBOX_VER < 200

class CFriends;

extern XONLINE_ACCEPTED_GAMEINVITE GGameInvite;
extern bool GInvited;

class DisplayFeedback : public Display
{
protected:
  RString _name;
  int _wantedExit;

public:
  DisplayFeedback(ControlsContainer *parent, RString name, RString resource);
  Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
  void OnButtonClicked(int idc);
  void OnChildDestroyed(int idd, int exit);
};

//! Xbox Friends display
class DisplayFriends : public Display
{
protected:
	InitPtr<CFriends> _friends;
	Ref<CStructuredText> _status;

	XONLINETASK_HANDLE _task;

  bool _warnIfExit;

public:
	//! constructor
	/*!
		\param parent parent display
	*/
	DisplayFriends(ControlsContainer *parent, bool warnIfExit);
	//! destructor
	~DisplayFriends();

	Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
	void OnSimulate(EntityAI *vehicle);

  void OnButtonClicked(int idc);
	void OnLBSelChanged(IControl *ctrl, int curSel);

	void OnChildDestroyed(int idd, int exit);

protected:
  void SendInvitation(XONLINE_MSG_HANDLE msg);
  void AcceptGameInvitation();
  void JoinGame();
};

#if _ENABLE_XHV

/// XHV implementation
class VoiceMailEngine : public ITitleXHV
{
protected:
  bool _recording;
  bool _playing;

  bool _recordingEnabled;
  bool _playbackEnabled;

  DWORD _start;
  AutoArray<char> _data;
  int _length;

  XHVEngine *_engine;
  int _port; // local port where recording / playing processed
  
  /// sometimes we need to wait until communication is established
  int _waitForCommunicator;

public:
  VoiceMailEngine();
  virtual ~VoiceMailEngine();

  const char *GetData() const {return _data.Data();}
  int GetSize() const {return _data.Size();}
  int GetDuration() const {return _length;}

  bool CanRecord() const {return _engine != NULL && _port >= 0 && _recordingEnabled;}
  bool CanReplay() const {return _engine != NULL && _port >= 0 && _playbackEnabled;}

  void OnSimulate();

  void RecordingStart();
  void RecordingStop();
  void ReplayStart();
  void ReplayStop();

  // ITitleXHV methods
  STDMETHODIMP CommunicatorStatusUpdate(DWORD dwPort, XHV_VOICE_COMMUNICATOR_STATUS status);
  STDMETHODIMP VoiceMailDataReady(DWORD dwLocalPort, DWORD dwDuration, DWORD dwSize);
  STDMETHODIMP VoiceMailStopped(DWORD dwLocalPort);

protected:
  void CreateVoiceEngine();
  void DestroyVoiceEngine();
  void SetPort(int port);

  virtual void UpdateDuration(float duration) = NULL;

  virtual void OnRecorded() {}
  virtual void OnReplayed() {}
};

#else

class VoiceMailEngine
{
protected:
  bool _recording;
  bool _playing;
  DWORD _start;
  AutoArray<char,MemAllocSafe> _data;
  int _length;

public:
  VoiceMailEngine();
  virtual ~VoiceMailEngine();

  const char *GetData() const {return _data.Data();}
  int GetSize() const {return _data.Size();}
  int GetDuration() const {return _length;}

  bool CanRecord() const {return true;}
  bool CanReplay() const {return true;}

  void OnSimulate();

  void RecordingStart();
  void RecordingStop();
  void ReplayStart();
  void ReplayStop();

protected:
  virtual void UpdateDuration(float duration) = NULL;

  virtual void OnRecorded() {}
  virtual void OnReplayed() {}
};

#endif

class DisplaySendVoiceMail : public Display, public VoiceMailEngine
{
protected:
  Ref<CStructuredText> _record;
  Ref<CStructuredText> _play;
  Ref<CStructuredText> _send;
  Ref<CProgressBar> _progress;
  Ref<CStructuredText> _time;

public:
  //! constructor
  /*!
  \param parent parent display
  */
  DisplaySendVoiceMail(ControlsContainer *parent, RString title, RString name);
  Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
  void OnSimulate(EntityAI *vehicle);
  void OnButtonClicked(int idc);

protected:
  void UpdateButtons();
  void UpdateDuration(float duration);

  virtual void OnRecorded();
  virtual void OnReplayed();
};

class DisplayReceiveVoiceMail : public Display, public VoiceMailEngine
{
protected:
  Ref<CStructuredText> _play;
  Ref<CProgressBar> _progress;
  Ref<CStructuredText> _time;

  RString _name;
  XUID _xuid;
  bool _played;
public:
  //! constructor
  /*!
  \param parent parent display
  */
  DisplayReceiveVoiceMail(ControlsContainer *parent, const XUID &xuid, RString name);
  Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
  void OnSimulate(EntityAI *vehicle);
  void OnButtonClicked(int idc);
  void OnChildDestroyed(int idd, int exit);

protected:
  bool ReadVoiceMail(DWORD msgID);
  void UpdateButtons();
  void UpdateDuration(float duration);
};

#endif // _XBOX_VER < 200

class DisplayLiveStats : public Display
{
protected:
  InitPtr<CListBoxContainer> _boards;

public:
  //! constructor
  /*!
  \param parent parent display
  */
  DisplayLiveStats(ControlsContainer *parent);

  Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
  void OnButtonClicked(int idc);
  void OnLBSelChanged(IControl *ctrl, int curSel);
};

#define MAX_STAT_USERS 10 // number of rows shown at once
class CLiveStatsList;

/// info about the statistics column
struct LiveStatsBoardColumn
{
  int _id; // column ID
  RString _name; // title
  float _width; // column width
  RString _format; // formatting string
  int _align; // text alignment
};
TypeIsMovableZeroed(LiveStatsBoardColumn)

/// all info needed to show the statistics table
struct LiveStatsBoardInfo
{
  /// list of columns in the leaderboard
  AutoArray<LiveStatsBoardColumn> _columns;

  /// user id of the local player
  XUID _player;
  /// name of the local player
  RString _playerName;

  /// friends enumeration handle
  HANDLE _friendsHandle;
  /// friends enumeration operation
  XOVERLAPPED _friendsOverlapped;
  /// number of friends in the list
  DWORD _friendsCount;
  /// buffer containing the list of friends
  Buffer<BYTE> _friendsBuffer;

  /// statistics update in progress
  bool _statsUpdating;
  /// statistics enumeration handle
  HANDLE _statsHandle;
  /// statistics read / enumeration operation
  XOVERLAPPED _statsOverlapped;
  /// buffer containing the statistics result
  Buffer<BYTE> _statsBuffer;
  /// the list of XUIDs we are receiving statistics for
  AutoArray<XUID> _xuids;
  /// the list of columns in the leaderboard
  XUSER_STATS_SPEC _spec;
  /// total number of players in the leaderboard
  DWORD _total;
  /// number of players in current view (list)
  DWORD _listRows;
  /// current position in the leaderboard
  DWORD _pageStart;

  /// text color of the player's row
  PackedColor _colorPlayer;
  /// text color of the friend's row
  PackedColor _colorFriend;

  /// start the enumeration of friends
  bool StartEnumFriends();
  /// cancel the enumeration of friends
  void StopEnumFriends();
  /// process the enumeration of friends
  bool ProcessEnumFriends();

  /// initialize the statistics description
  void StatsInit(int board);
  /// start reading of statistics for friends
  bool StartStatsFriends();
  /// start reading of statistics by rank
  bool StartStatsByRank(int rank);
  /// start reading of statistics by xuid
  bool StartStatsByXuid(XUID xuid);
  /// cancel reading of statistics
  void StopStats();
  /// process reading of statistics
  bool ProcessStats();
  /// sort the statistics by the rank
  void Sort();
  /// returns players position in current list or returns -1
  int GetPlayerPosition();

  /// check if given player is the local player
  bool IsPlayer(XUID xuid) const {return XOnlineAreUsersIdentical(xuid, _player) ? true : false;}
  /// check if given player is the local player's friend
  bool IsFriend(XUID xuid) const;
};

class DisplayLiveStatsBoard : public Display
{
protected:
  /// control - leaderboard list
  InitPtr<CLiveStatsList> _list;
  /// control - number of players in the leaderboard
  Ref<CStructuredText> _count;
  
  /// leaderboard ID
  int _board;
  /// filter only friends
  bool _friendsOnly;
  /// player is present in the current leaderboard
  bool _playerEnabled;
  // select player when list is updated
  bool _selectPlayer;

  /// the info needed for draw the statistics table content
  LiveStatsBoardInfo _info;

  /// table navigation command
  enum BoardPos
  {
    BPBegin,
    BPEnd,
    BPPrev,
    BPNext,
    BPPlayer
  };
public:
  //! constructor
  /*!
  \param parent parent display
  */
  DisplayLiveStatsBoard(ControlsContainer *parent, RString name);
  ~DisplayLiveStatsBoard();

  Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
  void OnSimulate(EntityAI *vehicle);
  void OnButtonClicked(int idc);

private:
  void StatsGoTo(BoardPos pos);
  void ShowGamerCard();
  void UpdateButtons();
};

// Downloadable content is handled by Xbox Guide on Xbox 360
#if _XBOX_VER < 200

TypeIsSimple(XONLINEOFFERING_INFO)
// TypeIsSimple(XOFFERING_ID)

//! Xbox Download Content Display
class DisplayDownloadContent : public Display
{
protected:
	InitPtr<CListBoxContainer> _list;
	AutoArray<XONLINEOFFERING_INFO> _info;
  AutoArray<XOFFERING_ID> _downloaded;
  AutoArray<XOFFERING_ID> _corrupted;
	XONLINETASK_HANDLE _task;
	Buffer<BYTE> _buffer;

  Ref<Texture> _none;
  Ref<Texture> _done;
  Ref<Texture> _bad;

public:
	DisplayDownloadContent(ControlsContainer *parent);
	~DisplayDownloadContent();

	Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
	void OnButtonClicked(int idc);
	void OnSimulate(EntityAI *vehicle);

  void OnChildDestroyed(int idd, int exit);

protected:
  void UpdateDownloaded();
  bool IsDownloaded(const XOFFERING_ID &id) const;
  bool IsCorrupted(const XOFFERING_ID &id) const;
};

//! Xbox Download Content Details Display
class DisplayDownloadContentDetails : public Display
{
protected:
	XONLINEOFFERING_INFO _info;
	XONLINEOFFERING_DETAILS _details;

	XONLINETASK_HANDLE _task;
	Buffer<BYTE> _buffer;

  QFBank _bank;

	InitPtr<ITextContainer> _title;
  Ref<CStatic> _picture;
	// Ref<CStructuredText> _price;
	Ref<CStructuredText> _terms;

  RString _name;
	bool _ok;
  bool _canDownload;
  int _restart;

public:
	DisplayDownloadContentDetails(ControlsContainer *parent, const XONLINEOFFERING_INFO &info, bool canDownload);
	~DisplayDownloadContentDetails();

	Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
	void OnButtonClicked(int idc);
	void OnSimulate(EntityAI *vehicle);
  void OnDraw(EntityAI *vehicle, float alpha);

	void OnChildDestroyed(int idd, int exit);

protected:
	void Install();
	bool Purchase();
	bool Unsubscribe();
	bool IsBillingEnabled() const;

  void UpdateButtons()
};

//! Xbox Download Content Price Display
class DisplayDownloadContentPrice : public Display
{
protected:
  InitPtr<ITextContainer> _title;
  Ref<CStructuredText> _price;
  InitPtr<ITextContainer> _question;

public:
  DisplayDownloadContentPrice(ControlsContainer *parent, RString title, RString price, RString question, int idd = -1);

  Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
  void OnButtonClicked(int idc);
};

class DisplayDownloadContentInstall : public Display
{
protected:
	XONLINETASK_HANDLE _task;

  Ref<CProgressBar> _progress;
  Ref<CAnimatedTexture> _time;
  InitPtr<ITextContainer> _title;

public:
	DisplayDownloadContentInstall(ControlsContainer *parent, const XOFFERING_ID &id, RString name);
	~DisplayDownloadContentInstall();

	Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
	void OnButtonClicked(int idc);
	void OnSimulate(EntityAI *vehicle);

	void OnChildDestroyed(int idd, int exit);
};

#endif // _XBOX_VER < 200

#endif

class CPlayers;

typedef bool AutoCancelFunction();

//! Xbox Players display
class DisplayXPlayers : public Display
{
protected:
	InitPtr<CPlayers> _players;
	Ref<CStructuredText> _status;

#if _XBOX_SECURE && _ENABLE_MP

  // Friends are handled by Xbox Guide on Xbox 360
#if _XBOX_VER < 200
	XONLINETASK_HANDLE _taskFriendList;
#endif

	XUID _selected;
  RString _nameSelected;

#if _XBOX_VER >= 200

#else
	XONLINETASK_HANDLE _taskMuteList;
#endif
	bool _muteListNeedUpdate;

  // Feedback is handled by Xbox Guide on Xbox 360
#if _XBOX_VER < 200
	XONLINETASK_HANDLE _taskFeedback;
#endif

#endif // _XBOX_SECURE && _ENABLE_MP

  AutoCancelFunction *_autoCancel;

public:
	//! constructor
	/*!
		\param parent parent display
	*/
	DisplayXPlayers(ControlsContainer *parent, AutoCancelFunction *autoCancel = NULL);
	//! destructor
	~DisplayXPlayers();

	Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);

	void OnButtonClicked(int idc);
	void OnLBSelChanged(IControl *ctrl, int curSel);
	void OnSimulate(EntityAI *vehicle);

	void OnChildDestroyed(int idd, int exit);

  // Friend requests are handled by Xbox Guide on Xbox 360
#if _XBOX_SECURE && _XBOX_VER < 200 && _ENABLE_MP
  void SendFriendRequest(XONLINE_MSG_HANDLE msg);
#endif

protected:
  /// Update button text by the context
  void UpdateButtons();
};

class MultiplayerSetupMessage;

class MultiplayerLauncher
{
public:
  MultiplayerLauncher();
  ~MultiplayerLauncher();

  //! Server functions
  void SaveLastMission();

  //! Client functions
  void InitGameInfo();

  //! returns true to continue other OnSimulate stuff
  bool OnSimulate();
  
  void ResetInfo()
  {
    _loadIsland = true;
  }

  void SetMessage(RString msg, int current, int total) 
  {
    _messageText = msg;
    _messageCurrent = current;
    _messageTotal = total;
  }
  virtual void SetErrMsgBox(UserFileErrorInfo &info) = 0;
  virtual void ClickButton(int idc) = 0;
  virtual void CreateChildML(Display *display) = 0;
  virtual Display *GetDisplayML() = 0;

private:
  //@{
  //! init state info
	bool _loadIsland;
  bool _delayedStart;
	//@}
public:
  Ref<ProgressHandle> _progressML;

  //@{ 
  //! displayed message
  SRef<MultiplayerSetupMessage> _msg;

  //! message text
  RString _messageText;
  int _messageTotal;
  int _messageCurrent;
  //@}
};

//! First server display
/*!
	Enables select (island and) multiplayer mission and sets difficulty.
*/
class DisplayServer : public Display, public MultiplayerLauncher
{
  typedef MultiplayerLauncher baseML;
protected:
	InitPtr<CListBoxContainer> _islands;
	InitPtr<CListBoxContainer> _missions;
  InitPtr<CListBoxContainer> _difficulties;

  InitPtr<CSliderContainer> _privateSlots;
  InitPtr<CSliderContainer> _maximumSlots;

  InitPtr<CHTMLContainer> _overview;

  AutoArray<MPMissionInfo> _missionList;

  PackedColor _colorEditor;
  PackedColor _colorWizard;
  PackedColor _colorMission;
  PackedColor _colorMissionEditor;
  PackedColor _colorMissionWizard;

  SaveGameType _saveType;

public:
	//! difficulty
	int _difficulty;
	
	//! constructor
	/*!
		\param parent parent display
	*/
	DisplayServer(ControlsContainer *parent);

	Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
	void OnButtonClicked(int idc);
	void OnLBSelChanged(IControl *ctrl, int curSel);
	void OnLBDblClick(int idc, int curSel);
	void OnChildDestroyed(int idd, int exit);
	void OnSimulate(EntityAI *vehicle);
  void OnDraw(EntityAI *vehicle, float alpha);

#if _GAMES_FOR_WINDOWS || (defined _XBOX && _XBOX_VER >= 200)
  /// handle invitation
  virtual void OnInvited();
#endif

  //{@ MultiplayerLauncher overrides
  virtual void SetErrMsgBox(UserFileErrorInfo &info) {}
  virtual void ClickButton(int idc) {}
  virtual void CreateChildML(Display *display) 
  {
    CreateChild(display); 
  }
  virtual Display *GetDisplayML() { return this; }
  //@}

// implementation
protected:
	//! updates list of missions (for example if selected island changes)
	void UpdateMissions(RString filename = RString(), int value = 2);
	//! updates buttons and tooltip when mission changes
	void OnMissionChanged(int sel);
  //! update mission info UI (overview, respawn, gametype, maxplayers)
  void UpdateMissionInfoUI(MPMissionInfo &info);
	//! sets selected mission as current mission (in framework)
	/*!
		\param editor editor will be performed
	*/
	bool SetMission(bool editor);

  /// Play the mission (Restart for idc==IDC_SERVER_LOAD)
  void OnPlayMission(int idc);

	//! updates display (buttons) when difficulty changed
	void OnChangeDifficulty();

	//! load options for display from user profile
	void LoadParams();
	//! save options for display to user profile
	void SaveParams();
  //! report the mission is corrupted
  void ReportCorrupted(const MPMissionInfo &info);
  //! search, where are stored save files for given mission
#ifdef _XBOX
  XSaveGame GetMissionSave(RString dir) const;
#else
  RString FindMissionSaveDirectory(RString dir) const;
#endif

#if _GAMES_FOR_WINDOWS || (defined _XBOX && _XBOX_VER >= 200)
  /// handle storage device change
  virtual void OnStorageDeviceChanged();
  /// saves has been changed
  virtual void OnSavesChanged();
#endif

};

// DisplayMultiplayer tweaked for scripted command HostMission purposes (dialog never shown, only creates its child dialogs)
class DisplayMultiplayerHostMission : public DisplayMultiplayer
{
private:
  ConstParamEntryPtr _cfg;
public:
  DisplayMultiplayerHostMission(ControlsContainer *parent, ConstParamEntryPtr cfg);
  void OnChildDestroyed(int idd, int exit);
};

#if _ENABLE_MISSION_CONFIG
// DisplayServer tweaked for for scripted command HostMission purposes (dialog never shown, only automatically creates DisplayMultiplayerSetup dialog with selected mission)
class DisplayServerHostMission : public DisplayServer
{
public:
  DisplayServerHostMission(ControlsContainer *parent, ConstParamEntryPtr cfg);
  void OnChildDestroyed(int idd, int exit);
  void OnPlayMission(ConstParamEntryPtr cfg);
};
#endif

//! Gamemaster select mission display
/*!
	Enables select (island and) multiplayer mission and sets difficulty on remote dedicated server.
*/
class DisplayRemoteMissions : public Display
{
protected:
  InitPtr<CListBoxContainer> _difficulties;
  InitPtr<CHTMLContainer> _overview;

public:
	//! difficulty
	int _difficulty;

	//! constructor
	/*!
		\param parent parent display
	*/
	DisplayRemoteMissions(ControlsContainer *parent);

	Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
	void OnButtonClicked(int idc);
	void OnLBSelChanged(IControl *ctrl, int curSel);
	void OnLBDblClick(int idc, int curSel);
	void OnSimulate(EntityAI *vehicle);

// implementation
protected:
	//! updates list of missions (for example if selected island changes)
	void UpdateMissions(RString filename = "");
	//! updates buttons and tooltip when mission changes
	void OnMissionChanged(int sel);
};

//! Gamemaster select (or vote) mission display for Xbox
/*!
Enables select multiplayer mission and sets difficulty on remote dedicated server.
*/
class DisplayXRemoteMissions : public Display
{
public:
  InitPtr<CListBoxContainer> _gameTypes;
  InitPtr<CListBoxContainer> _missions;
  InitPtr<CListBoxContainer> _difficulties;

  InitPtr<ITextContainer> _island;
  InitPtr<ITextContainer> _minPlayers;
  InitPtr<ITextContainer> _maxPlayers;

  // AutoArray<MPMissionInfo> _missionList;
  int _difficulty;

  PackedColor _sliderColor;
  PackedColor _sliderColorActive;

  //! constructor
  /*!
  \param parent parent display
  */
  DisplayXRemoteMissions(ControlsContainer *parent);

  Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
  void OnButtonClicked(int idc);
  void OnLBSelChanged(IControl *ctrl, int curSel);
  void OnLBSelUnchanged(int idc, int curSel);
  void OnLBListSelChanged(int idc, int curSel);
  void OnSimulate(EntityAI *vehicle);

  RString GetMission() const;
  int GetDifficulty() const {return _difficulty;}

  // implementation
protected:
  //! updates list of missions (for example if selected island changes)
  void UpdateMissions(RString filename = "");
  //! updates informations about mission
  void UpdateMissionInfo(int index);
  //! updates buttons and tooltip when mission changes
  void OnMissionChanged();
};

//! dedicated server mission voting is done, show progress
class DisplayXVotedMissions : public Display
{
  typedef Display base;
  
public:
  //! constructor
  /*!
  \param parent parent display
  */
  DisplayXVotedMissions(ControlsContainer *parent);

  Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
  void OnButtonClicked(int idc);
  void OnSimulate(EntityAI *vehicle);

};

#ifndef _XBOX

//! IP Address display (enter ip address for session list)
class DisplayIPAddress : public Display
{
public:
	//! constructor
	/*!
		\param parent parent display
	*/
	DisplayIPAddress(ControlsContainer *parent)
		: Display(parent)
	{
		_enableSimulation = false;
		Load("RscDisplayIPAddress");
	}
	bool CanDestroy();
	Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
};

#endif

//! First client display
/*!
	Displays text "Wait for server". Checks if server is ready (mission is selected).
*/
class DisplayClient : public Display, public MultiplayerLauncher
{
  typedef MultiplayerLauncher baseML;
protected:
  Ref<CProgressBar> _progress;

public:
	//! constructor
	/*!
		\param parent parent display
	*/
	DisplayClient(ControlsContainer *parent)
		: Display(parent), MultiplayerLauncher()
	{
		_enableSimulation = false;
		Load("RscDisplayClient");
		SetCursor(NULL);
	}

  Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
	void OnButtonClicked(int idc);
	void OnChildDestroyed(int idd, int exit);
	void OnSimulate(EntityAI *vehicle);

#if _GAMES_FOR_WINDOWS || (defined _XBOX && _XBOX_VER >= 200)
  /// handle invitation
  virtual void OnInvited();
#endif

  //{@ MultiplayerLauncher overrides
  virtual void SetErrMsgBox(UserFileErrorInfo &info) {}
  virtual void ClickButton(int idc) {}
  virtual void CreateChildML(Display *display) 
  {
    CreateChild(display); 
  }
  virtual Display *GetDisplayML() { return this; }
  //@}
};

// Server and client briefing

//! Mission briefing for server
/*!
	Shows list of players with ready / not ready state.
*/
class DisplayServerGetReady : public DisplayGetReady
{
	typedef DisplayGetReady base;

protected:
  bool _launched;
  PackedColor _color0;
  PackedColor _color1;
  PackedColor _color2;

public:
	//! constructor
	/*!
		\param parent parent display
	*/
	DisplayServerGetReady(ControlsContainer *parent);
	void OnButtonClicked(int idc);
	void OnChildDestroyed(int idd, int exit);
	void Destroy();
	void OnSimulate(EntityAI *vehicle);

protected:
  void Launch();
};

//! Mission briefing for server
/*!
	Enables send ready state.
*/
class DisplayClientGetReady : public DisplayGetReady
{
	typedef DisplayGetReady base;

protected:
  bool _launched;
  PackedColor _color0;
  PackedColor _color1;
  PackedColor _color2;

public:
	//! constructor
	/*!
		\param parent parent display
	*/
	DisplayClientGetReady(ControlsContainer *parent);
	void Destroy();
	void OnButtonClicked(int idc);
  void OnChildDestroyed(int idd, int exit);
	void OnSimulate(EntityAI *vehicle);

protected:
  void Launch();
};

#ifndef _XBOX
//! Multiplayer players display
/*!
	Shows list of all players.
	For selected player shows detailed info (from squad description site).
*/
class DisplayMPPlayers : public Display
{
protected:
	//@{
	//! colors of players in list
	PackedColor _color;
	PackedColor _colorWest;
	PackedColor _colorEast;
	PackedColor _colorCiv;
	PackedColor _colorRes;
	//@}

public:
	//! constructor
	/*!
		\param parent parent display
		\param init enable initialization (controls creation from template)
	*/
	DisplayMPPlayers(ControlsContainer *parent, bool init = true)
	: Display(parent)
	{
    GInput.ChangeGameFocus(+1); //call it as early as possible in order not to skip it by some possible return
		if (init)
		{
			Load("RscDisplayMPPlayers");
			UpdatePlayers();
			UpdatePlayerInfo();
		}
		
		ParamEntryVal cls = Pars >> "CfgInGameUI" >> "MPTable";
		_color = GetPackedColor(cls >> "color");
		_colorWest = GetPackedColor(cls >> "colorWest");
		_colorEast = GetPackedColor(cls >> "colorEast");
		_colorCiv = GetPackedColor(cls >> "colorCiv");
		_colorRes = GetPackedColor(cls >> "colorRes");
	}
	//! destructor
	~DisplayMPPlayers()
	{
		GInput.ChangeGameFocus(-1);
	}

	void OnButtonClicked(int idc);
	void OnLBSelChanged(IControl *ctrl, int curSel);
	void OnSimulate(EntityAI *vehicle);

protected:
	//! update list of players (called in each simulation step)
	void UpdatePlayers();
	//! update detailed player info (called if selected player changes)
	void UpdatePlayerInfo();
};

#endif

#ifndef _XBOX
//! Login display (used for selection of current user)
class DisplayLogin : public Display
{
protected:
	//! wanted exit code (exit is performed after close animation is finished)
	int _exitWhenClose;

public:
	//! constructor
	/*!
		\param parent parent display
	*/
	DisplayLogin(ControlsContainer *parent)
		: Display(parent)
	{
		Load("RscDisplayLogin");
	}

	Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
	void OnButtonClicked(int idc);
	void OnLBSelChanged(IControl *ctrl, int curSel);
	void OnCtrlClosed(int idc);
	void OnChildDestroyed(int idd, int exit);

	bool CanDestroy();
};
#endif

/*
Character Creation (used for creating characters)
*/
class DisplayCharacter : public Display
{
protected:
	//! wanted exit code (exit is performed after close animation is finished)
	int _exitWhenClose;
public:
	/*!
		\param parent parent display
	*/
	DisplayCharacter(ControlsContainer *parent)
		: Display(parent)
	{
		//Load("RscDisplayNewSurvivor");
		Load("RscDisplayCharacter");
	}
	Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
	void OnButtonClicked(int idc);
	//void OnChildDestroyed(int idd, int exit);
};

class DisplayCharacterDefault : public Display
{
protected:

public:
	DisplayCharacterDefault(ControlsContainer *parent, bool enableSimulation);
	Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
	void OnButtonClicked(int idc);
};

class DisplayCharacterConfirm : public Display
{
protected:

public:
	DisplayCharacterConfirm(ControlsContainer *parent, bool enableSimulation);
};

class DisplayCharacterRestart : public Display
{
protected:

public:
	DisplayCharacterRestart(ControlsContainer *parent, bool enableSimulation);
};

#ifndef _XBOX

//! Mod Launcher Display (used for mods selection)
class DisplayModLauncher : public Display
{
protected:
  //! wanted exit code (exit is performed after close animation is finished)
  int _exitWhenClose;

  //! list of modes available for the game
  AutoArray<ModInfo> _modList;

  //! possible picture to present the selected mod
  InitPtr<CStatic> _picture;

  //! modList UI container
  InitPtr<CListBoxContainer> _modListContainer;

  //!icons to indicate given mod is active, enabled or disabled
  Ref<Texture> _iconActive;
  Ref<Texture> _iconEnabled;
  Ref<Texture> _iconDisabled;

  //!default text on action button
  RString _defaultActionText;
  RString _defaultAction;  //taken from defaultAction entry in CfgMods

public:
  // helper variable to pass mod for CollectMod directory traversing callback
  enum ModInfo::ModOrigin curCollectMod;
  RString   curRootDir;   //mods must be direct subdirectories of gameDir or myDocument game dir

  //! constructor
  /*!
  \param parent parent display
  */
  DisplayModLauncher(ControlsContainer *parent)
    : Display(parent)
  {
    _defaultActionText = "Action";
    Load("RscDisplayModLauncher");
    if ( _modListContainer && _modListContainer->Size()>0 ) _modListContainer->SetCurSel(0);
  }

  // Test whether given mod is present inside modList
  static int ModIndex(const ModInfo &mod, const AutoArray<ModInfo> &modList);

  Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
  void OnButtonClicked(int idc);
  void OnLBSelChanged(IControl *ctrl, int curSel);
  void OnCtrlClosed(int idc);
  void OnChildDestroyed(int idd, int exit);
  void OnLBDblClick(int idc, int curSel);

  bool CanDestroy();
  void DisableButtons(); // Disable manipulating buttons

  // Detect available mods in the supported places (gamedir, mydocument player profile dir)
  void ReloadModList();
  // Enable or Disable button, Mod related button, ...
  void RefreshButtons();
  // Add mod, but only when it is not added yet
  void Add(ModInfo info);
  // Swap items in the listboxitem list
  void SwapItems(int curSel,int curSel2,bool changeCurSel);
  // Serialize the enabled Mods into userCfg profile 
  void SaveModList();
  // Disable/Enable button or double click on mod action
  void ChangeModStatus();
  // Enable/Disable registry driven mod
  void EnableMod(RString modRegName, bool enable);
  // Enable/Disable mod on given position
  int EnableMod(int sel, bool enable);
  // Get mod list
  const AutoArray<ModInfo> &GetModList() { return _modList; }
  // get the highest index in range 0..modIx which matches the LOADAFTER conditions from registry
  int SortModIn(int modIx);
};
#endif

//! Control for preview of selected face
class CHead : public ControlObject, public LODShapeLoadHandler
{
protected:
#if _ENABLE_NEWHEAD
  /// head and glasses implemented as proxies
  ProxiesHolder _proxies;
  //! head type (shape, animations etc.)
  Ref<HeadType> _headType;
  //! head object (mimics etc.)
  Head _head;
  //! current face material
  Ref<TexMaterial> _faceMaterial;
  //! current face texture
  Ref<Texture> _faceTexture;
  /// selected glasses
  Ref<GlassesType> _glassesType;

  //!{ Bone names
  RString _boneHead;
  RString _boneLEye;
  RString _boneREye;
  RString _boneLEyelidUp;
  RString _boneREyelidUp;
  RString _boneLEyelidDown;
  RString _boneREyelidDown;
  RString _boneLPupil;
  RString _boneRPupil;
  //!}

#else
  //! head type (shape, animations etc.)
  SRef<HeadTypeOld> _headType;
  //! head objects (concrete face, mimics etc.)
  SRef<HeadOld> _head;
#endif

  //! head bode matrix indices
  SkeletonIndex _headIndex;

	//! time of last simulation step
	UITime _lastSimulation;

	//! model face Type
	RString _faceType;

  bool _rotate;

  //@{
  //! animation implementation
  UITime _animStart;
  Ref<AnimationRT> _animation;
  //@}

  //@{
  //! animation speed
  float _animSpeed;
  //@}

public:
	//! constructor
	/*!
		Used when control is created by resource template.
		\param parent control container by which this control is owned
		\param idc id of control
		\param cls resource template
	*/
	CHead(ControlsContainer *parent, int idc, ParamEntryPar cls);
	~CHead();

  // which face type is able to display
  RString GetFaceType() const {return _faceType;}

  void Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2);
	void Deanimate(int level, bool setEngineStuff);
  AnimationStyle IsAnimated(int level) const {return AnimatedGeometry;}
  bool PrepareShapeDrawMatrices(Matrix4Array &matrices, const ObjectVisualState &vs, LODShape *shape, int level);

#if _ENABLE_NEWHEAD
  void DrawProxies(
    int cb, const AnimationContext *animContext, int level, ClipFlags clipFlags,
    const Matrix4 &transform, float dist2, const DrawParameters &dp, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
    );
#endif

  //@{ LODShapeLoadHandler implementation
  virtual void LODShapeLoaded(LODShape *shape);
  virtual void LODShapeUnloaded(LODShape *shape);
  virtual void ShapeLoaded(LODShape *shape, int level);
  virtual void ShapeUnloaded(LODShape *shape, int level);
  //@}

  virtual void OnDraw(float alpha);
	//! single simulation step
	void Simulate();
	//! changes face
	void SetFace(RString name);
	//! changes glasses
	void SetGlasses(RString name);
	//! attaches lip info to head object
	void AttachWave(AbstractWave *wave, float freq = 1);

protected:
#if _ENABLE_NEWHEAD
  /// load the head model (contained in _headType)
  void SetHead(RString name);
  /// free the head model (contained in _headType)
  void RemoveHead();
  /// free the glasses model (contained in _glassesType)
  void RemoveGlasses();
  /// draw single proxy object (head, glasses)
  void DrawProxy(int cb, const DrawParameters &dp, Object *object, float dist2, const Matrix4 &transform, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif    
    );
#endif
};

/// Returns the first type of speaker defined in CfgVoiceTypes (used in initialization)
RString GetFirstSpeakerType();

#ifndef _XBOX

//! User properties dialog
/*!
	Enables change basic parameters of player (name, face, voice etc.).
*/
class DisplayNewUser : public Display
{
public:
	//! initial player's name
	RString _name;
	//! if edit existing user or create new
	bool _edit;

protected:
	//! if voice preview will be performed
	bool _doPreview;

	//! wanted exit code (exit is performed after close animation is finished)
	int _exitWhenClose;

	//! face name
	RString _face;
	//! glasses name
	RString _glasses;
	//! name of speaker class
	RString _speakerType;
	//! squad id
	RString _squad;
	//! pitch of voice
	float _pitch;

	//! time when preview will start (if no additional user changes will be performed)
	DWORD _previewTime;
	//! speaker class name to preview
	RString _previewSpeakerType;
	//! voice pitch to preview
	float _previewPitch;
	//! preview sound
	RefAbstractWave _previewSpeech;

	//! face preview control
	Ref<CHead> _head;
  //! enabled face type
  RString _faceType;

  //! list of faces
  InitPtr<CListBoxContainer> _faces;

public:
	//! constructor
	/*!
		\param parent parent display
	*/
	DisplayNewUser(ControlsContainer *parent, RString name, bool edit);

	Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
	ControlObject *OnCreateObject(int type, int idc, ParamEntryPar cls);
	void OnButtonClicked(int idc);
	void OnSliderPosChanged(IControl *ctrl, float pos);
	void OnLBSelChanged(IControl *ctrl, int curSel);
	void OnObjectMoved(int idc, Vector3Par offset);
	void OnCtrlClosed(int idc);
	void OnSimulate(EntityAI *vehicle);
	bool CanDestroy();

protected:
	//! starts voice preview
	void Preview(RString speakerType, float pitch);
  //! returns name "Player X", where X is not yet used number
  RString GetNewName();
};
#endif

//! Display performed when player was killed
class DisplayMissionEnd : public Display
{
protected:
	//! quotation index
	int _quotation;

  /// FileTime operation for saves (NULL when save does not exist)
  Future<QFileTime> _saveHandles[NSaveGameType];

  /// Selected user save slot (when exit with IDC_ME_LOAD)
  SaveGameType _selectedSlot;

public:
	//! constructor
	/*!
		\param parent parent display
	*/
	DisplayMissionEnd(ControlsContainer *parent, Future<QFileTime> saveTimeHandles[NSaveGameType]);
	//! destructor
	~DisplayMissionEnd();

  SaveGameType GetSelectedSlot() const {return _selectedSlot;}

	Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
	void OnButtonClicked(int idc);
  void OnChildDestroyed(int idd, int exit);

protected:
  /// updates save controls
  void UpdateSaveControls(Future<QFileTime> saveTimeHandles[NSaveGameType]);
#if _GAMES_FOR_WINDOWS || (defined _XBOX && _XBOX_VER >= 200)
  /// handle storage device change
  virtual void OnStorageDeviceChanged();
#endif
};

class DisplayMissionFail : public Display
{

protected:
  //! mission result
  InitPtr<ITextContainer> _result;
  //! mission name
  InitPtr<ITextContainer> _title;
  //! debriefing text
  InitPtr<CHTMLContainer> _debriefing;
  //! list of objectives
  InitPtr<CHTMLContainer> _overview;
  //! basic mission info
  InitPtr<CHTMLContainer> _info;
  //! statistics page control
  InitPtr<CHTMLContainer> _stats;
  //! statistics of last mission
  bool _animation;
  //! statistics of last mission
  AIStats _oldStats;

  bool _objectives;
  bool _server;
  bool _client;

#if _XBOX_SECURE && _XBOX_VER < 200 && _ENABLE_MP
  // SP statistics are not possible on Xbox 360
  bool _exiting;
  SRef<LiveStatsUpdate> _statsUpdate;
#endif
  int _wantedExit;

  //! quotation index
  int _quotation;

  /// FileTime operation for saves (NULL when save does not exist)
  Future<QFileTime> _saveHandles[NSaveGameType];

  /// Selected user save slot (when exit with IDC_ME_LOAD)
  SaveGameType _selectedSlot;

public:
  //! constructor
  /*!
  \param parent parent display
  */
  DisplayMissionFail(ControlsContainer *parent, Future<QFileTime> saveTimeHandles[NSaveGameType]);
  //! destructor
  ~DisplayMissionFail();

  SaveGameType GetSelectedSlot() const {return _selectedSlot;}

  Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
  void OnButtonClicked(int idc);
  void OnChildDestroyed(int idd, int exit);
  void CreateDebriefing();
  bool OnHTMLLink(int idc, RString link);

protected:
  /// updates save controls
  void UpdateSaveControls(Future<QFileTime> saveTimeHandles[NSaveGameType]);
#if _GAMES_FOR_WINDOWS || (defined _XBOX && _XBOX_VER >= 200)
  /// handle storage device change
  virtual void OnStorageDeviceChanged();
#endif
};


// Mission, intro, outro, campaign intro displays

//! "Fake" display used when outro cutscene is performed
/*!
	No display. Game is running "in background". Waits for user input.
*/
class DisplayOutro : public DisplayCutscene
{
public:
	//! end of game index (used for outro selection)
	EEndMode _mode;

public:
	//! constructor
	/*!
		\param parent parent display
		\param mode end of game index (used for outro selection)
	*/
	DisplayOutro(ControlsContainer *parent, EEndMode mode);
  ~DisplayOutro();
	virtual void PlayAgain();
};

//! Interrupt revert display (selects from different possibilities to revert)
class DisplayInterruptRevert : public Display
{
protected:
  InitPtr<CListBoxContainer> _revertTypes;

public:
  DisplayInterruptRevert(ControlsContainer *parent, Future<QFileTime> saveHandles[NSaveGameType]);

  /// return the save slot selected by the user
  SaveGameType GetSaveSlot() const;

  Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
  void OnButtonClicked(int idc);
  void OnLBDblClick(int idc, int curSel);

protected:
#if _GAMES_FOR_WINDOWS || (defined _XBOX && _XBOX_VER >= 200)
  /// handle storage device change
  virtual void OnStorageDeviceChanged();
  /// handle storage device removal
  virtual void OnStorageDeviceRemoved();
#endif
};

//! Select the user save slot
class DisplaySelectUserSave : public Display
{
protected:
  InitPtr<CListBoxContainer> _saveTypes;

public:
  DisplaySelectUserSave(ControlsContainer *parent, Future<QFileTime> saveHandles[NSaveGameType]);

  /// return the save slot selected by the user
  SaveGameType GetSaveSlot() const;

  Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
  void OnLBDblClick(int idc, int curSel);
protected:
#if _GAMES_FOR_WINDOWS || (defined _XBOX && _XBOX_VER >= 200)
  /// handle storage device change
  virtual void OnStorageDeviceChanged();
  /// handle storage device removal
  virtual void OnStorageDeviceRemoved();
#endif
};

//! Interrupt display (performed if mission is interruptet by user (ESCAPE key))
class DisplayInterrupt : public Display
{
protected:
	bool _request;
	bool _invitation;
  /// start simulation after button creating the dialog released
  bool _skipSimulation;
  
  /// FileTime operation for saves (NULL when save does not exist)
  Future<QFileTime> _saveHandles[NSaveGameType];

  /// Selected user save slot (when exit with IDC_INT_LOAD or IDC_INT_SAVE)
  SaveGameType _selectedSlot;

	Ref<CStructuredText> _friends;
  InitPtr<CStructuredText> _signInStatus;

public:
	//! constructor
	/*!
		\param parent parent display
	*/
	DisplayInterrupt(ControlsContainer *parent, Future<QFileTime> saveTimeHandles[NSaveGameType]);
  DisplayInterrupt(ControlsContainer *parent);
	//! destructor
	~DisplayInterrupt();

  SaveGameType GetSelectedSlot() const {return _selectedSlot;}

  /// initialization common to both constructors
  void Init();

	Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
	void OnButtonClicked(int idc);
	void OnSimulate(EntityAI *vehicle);
  bool OnKeyDown(int dikCode);
  bool OnKeyUp(int dikCode);
	void OnChildDestroyed(int idd, int exit);

protected:
	void UpdateButtons();
  /// updates save controls
  void UpdateSaveControls(Future<QFileTime> saveTimeHandles[NSaveGameType]);
  /// find the free user save slot
  SaveGameType GetFreeUserSlot() const;

#if _GAMES_FOR_WINDOWS || (defined _XBOX && _XBOX_VER >= 200)
  /// handle storage device change
  virtual void OnStorageDeviceChanged();
#endif
};

//! Mission editor interrupt display
class DisplayEditorInterrupt : public Display
{
public:
  //! constructor
  /*!
  \param parent parent display
  */
  DisplayEditorInterrupt(ControlsContainer *parent, RString resource);
  //! destructor
  ~DisplayEditorInterrupt();

  void OnButtonClicked(int idc);
  void OnChildDestroyed(int idd, int exit);
};

//! Cutscene interrupt display (performed if cutscene is interruptet by user (ESCAPE key))
class DisplayCutsceneInterrupt : public Display
{
public:
  //! constructor
  /*!
  \param parent parent display
  */
  DisplayCutsceneInterrupt(ControlsContainer *parent);
  //! destructor
  ~DisplayCutsceneInterrupt();

  void OnButtonClicked(int idc);
  void OnChildDestroyed(int idd, int exit);
};

struct VideoPlayback;

#ifdef _XBOX
struct AttractModeItem
{
  RString file;
  bool showMenu;
  float time;
};
TypeIsMovableZeroed(AttractModeItem)
#endif

//! Main menu display
class DisplayMain : public Display
{
protected:
	//! application version (read from windows resource)
	RString _version;
  //! mod icons controlsGroup
  CControlsGroup *_modIcons;

#if _VBS3
  DWORD   _reConnectDelay;
#endif

#ifdef _XBOX
  VideoPlayback *_decoder;
  AutoArray<AttractModeItem> _video;
  int _playing;
  RString _playingFile;
  DWORD _lastChange;
  DWORD _nextChange;

  bool _initialInteractionProcessed;

  /// index of last button clicked
  int _lastButtonClicked;
  /// if we should test user sign in OnButtonClicked
  bool _testSignIn;
  /// if we should test storage device in OnButtonClicked
  bool _testStorageDevice;
#endif
  bool _gameInitialized;

  bool _wantedMissionSkipBriefing;
  RString _wantedMission;

  // dialog we want to show if player ignore low disk space message
  int _wantedNext;

public:
	//! constructor
	/*!
		\param parent parent display
	*/
	DisplayMain(ControlsContainer *parent);
  ~DisplayMain();

	Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
	void OnButtonClicked(int idc);
	void OnChildDestroyed(int idd, int exit);
	void OnDraw(EntityAI *vehicle, float alpha);
  void OnSimulate(EntityAI *vehicle);
  void OnControllerRemoved();
  void OnEditChanged(int idc) override;

	void DestroyHUD(int exit);
  void StartUserMission(RString name);
  void StartWizardMission(RString filename);
  void StartMission(RString campaign, RString mission, bool skipBriefing);

  bool IsGameInitialized() const {return _gameInitialized;}
  void UpdatePlayerName();

protected:
  void EnableVideo(bool enable);
  void NextVideo(bool interrupted);
  bool IsMenuEnabled() const;

#ifdef _XBOX
  void OnInitialInteraction();
#endif

#if _GAMES_FOR_WINDOWS || (defined _XBOX && _XBOX_VER >= 200)
#ifdef _XBOX
  /// checks storage device and shows device selection UI if no device is selected
  bool CheckStorage();
#endif
  /// handle invitation
  virtual void OnInvited();
  /// handles return to main menu request
  virtual void OnReturnToMainMenuRequest();
#endif

  void DoStartMission(RString campaign, RString mission, bool skipBriefing);

#if _ENABLE_CHEATS || _ENABLE_AUTOTEST
  class TestMissions
  {
  public:
    struct TestMission
    {
      RString campaign;
      RString mission;
      EEndMode end;
      float started;
      float ended;

      struct SlowFrameInfo
      {
        float time;
        float duration;
        RString log1;
        RString log2;
      };

      SlowFrameInfo slowFrames[10];
      int slowFramesCount;

      void Load(ParamEntryPar cls)
      {
        campaign = cls >> "campaign";
        mission = cls >> "mission";
        ConstParamEntryPtr entry = cls.FindEntry("end");
        if (entry)
        {
          RStringB name = *entry;
          end = GetEnumValue<EEndMode>(name);
        }
        else end = EMEnd1;
      }
      ClassIsMovableZeroed(TestMission);
    };
    
    AutoArray<TestMission> testMissions;
    int ix;
    int failedCount;
    bool failed;

  public:
    TestMissions() { ix=0; failedCount=0; failed=false; };
    bool Active() const { return ( testMissions.Size()>0 ); };
    void Next() { ix++; }
    bool IsMission() { return ( ix < testMissions.Size() ); }
    void SetFailed() { failed=true; }
    bool Failed() const { return failed; }
    const RString &GetCampaign() { return testMissions[ix].campaign; }
    const RString &GetMission() { return testMissions[ix].mission; }
    const EEndMode GetEnd() { return testMissions[ix].end; }

    TestMission *GetInfo() {return ix < testMissions.Size() ? &testMissions[ix] : NULL;}
  };

  TestMissions _testMissions;

public:
  void AutotestStartMission()
  {
    TestMissions::TestMission *info = _testMissions.GetInfo();
    if (info)
    {
      info->started = 0.001f * GlobalTickCount();
      info->slowFramesCount = 0; // ignore all frames prior mission start
    }
  }
  void AutotestEndMission()
  {
    TestMissions::TestMission *info = _testMissions.GetInfo();
    if (info)
    {
      info->ended = 0.001f * GlobalTickCount();
    }
  }
  void AutotestFrame(float duration, RString log1, RString log2);
#endif
};

#if _VBS2
class DisplayMainVBS : public DisplayMain
{
public:
  DisplayMainVBS(ControlsContainer *parent);

  void OnChildDestroyed(int idd, int exit);
  void OnButtonClicked(int idc);
};
#endif

#if _ENABLE_CHEATS

const int DebugConsoleExpRows = 4;
struct DebugConsoleInfo
{
  AutoArray<RString> history;
  RString exp[DebugConsoleExpRows];
  RString filtrRule;

  LSError Serialize(ParamArchive &ar);
};

//! Debug console display
class DisplayDebug : public Display
{
protected:
  //! Persistent history
  DebugConsoleInfo _debugInfo;
	//! index of expression in expressions history list
	int _current;
  //! variable space valid in the debug console
  GameVarSpace _vars;
  //! gamestate this console is working with
  GameState *_gs;

public:
	//! constructor
	/*!
		\param parent parent display
	*/
	DisplayDebug(ControlsContainer *parent, GameState *gs, const char *cls = "RscDisplayDebug" );
	//! destructor
	~DisplayDebug();
	Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
	void OnButtonClicked(int idc);
	bool OnKeyDown(int dikCode);
  void OnEditChanged(int idc);//used for log filtering
	void OnDraw(EntityAI *vehicle, float alpha);

	void DestroyHUD(int exit);

protected:
	//! updates log list
	void UpdateLog(CListBox *lbox);
  void LoadHistory();
  void SaveHistory();
  //!check if given string match filtering rule
  bool FilterLogTest(RString text);
};

#endif

//! Debug console display
class DisplayDSInterface : public Display
{
protected:
   //! gamestate this console is working with
  InitPtr<CListBoxContainer> _lboxMissions;
  InitPtr<CListBoxContainer> _lboxPlayers;

public:
  //! constructor
  /*!
  \param parent parent display
  */
  DisplayDSInterface(ControlsContainer *parent, const char *cls = "RscDisplayDSinterface" );
	Control *OnCreateCtrl(int type, int idc, ParamEntryPar cls);
  void OnSimulate(EntityAI *vehicle);
	void DestroyHUD(int exit);
  ~DisplayDSInterface();

  void UpdateMissionsList();
  void UpdatePlayers();
};

RString FindPicture(RString name);
RString GetUserDirectory();
RString GetSaveDirectory();

bool ProcessTemplateName(RString name);
bool ProcessTemplateName(RString name, RString dir);
bool ParseMission(bool multiplayer, bool avoidCheckIds = false, bool allowEmpty = false);
bool ParseIntro(bool avoidCheckIds = false);
bool ParseCutscene(RString name, bool multiplayer, bool avoidCheckIds = false, bool allowEmpty = false);

void RunInitScript();

extern bool ContinueSaved;

void StartRandomCutscene(RString world, bool runInitSqs=true);

bool OnIntroFinished(Display *disp, int exit, bool enableEmptyMission = false, bool skipBriefing = false);
bool OnBriefingFinished(Display *disp, int exit);
bool OnMissionFinished(Display *disp, int exit, bool restartEnabled);
bool OnOutroFinished(Display *disp, int exit);
#endif
