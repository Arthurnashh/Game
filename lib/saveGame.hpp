#ifdef _MSC_VER
#pragma once
#endif

#ifndef _SAVE_GAME_HPP
#define _SAVE_GAME_HPP

#define CHECK_SPACE_PROFILE   5
#define CHECK_SPACE_MISSION   10
#define CHECK_SPACE_SAVE      1000
#define CHECK_SAVE_BYTES      40000000


#if _GAMES_FOR_WINDOWS

#include <Es/common/win.h>

#include <Es/Strings/rString.hpp>

#ifdef _XBOX // save system specific to Xbox 360, overlapped operations usable also for Games for Windows - LIVE

//////////////////////////////////////////////////////////////////////////
//
// Generic save system
//

/// symbolic name for drive for reading/writing user saves
#define SAVE_DRIVE  "save"
#define SAVE_ROOT   SAVE_DRIVE ":\\"

/// symbolic name for drive for reading/writing settings
#define SAVE_SETTINGS_DRIVE  "savesettings"
/// root path for drive for reading/writing settings
#define SAVE_SETTINGS_ROOT   SAVE_SETTINGS_DRIVE ":\\"
/// name of the content package file
#define SAVE_SETTINGS_CONTENT_FILE_NAME "settings"
///  display name of the content package file
#define SAVE_SETTINGS_DISPLAY_NAME L"User settings"
/// file name of the settings file
#define SAVE_SETTINGS_FILE_NAME SAVE_SETTINGS_ROOT "settings.bin"

#define MARKETPLACE_DRIVE_FORMAT "MP%08x"


#include <Es/Memory/normalNew.hpp>
#include <Es/Containers/Array.hpp>

class QIStrStream;
class QOStrStream;

class XSaveGameImpl : public RefCount, public NoCopy
{
  friend class SaveSystem;

private:
  static int _instances;

  XSaveGameImpl();

public:
  virtual ~XSaveGameImpl();

  USE_FAST_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>

TypeIsSimple(XCONTENT_DATA);

/// The encapsulation of Xbox 360 save
typedef Ref<XSaveGameImpl> XSaveGame;

/// The description of save on the Xbox 360
struct SaveDescription
{
  RString fileName;
  RString displayName;
};
TypeIsMovableZeroed(SaveDescription)

#endif // defined _XBOX

#include <Es/Memory/normalNew.hpp>

/// Base class for overlapped operations
class OverlappedOperationBase: public RefCount
{
public:
  virtual ~OverlappedOperationBase() {}

  /// regular processing
  virtual HRESULT Process() = 0;
  /// processed when the operation succeeded
  virtual void OnSucceeded() = 0;
  /// processed when the operation failed
  virtual void OnFailed() = 0;

  USE_FAST_ALLOCATOR
};

/// The encapsulation of XOVERLAPPED operation
class OverlappedOperation : public OverlappedOperationBase
{
protected:
  XOVERLAPPED _overlapped;

public:
  OverlappedOperation();
  virtual ~OverlappedOperation();

  /// regular processing
  virtual HRESULT Process();
  /// processed when the operation succeeded
  virtual void OnSucceeded() {}
  /// processed when the operation failed
  virtual void OnFailed() {}

  USE_FAST_ALLOCATOR
};

/// the encapsulation of classic IO OVERLAPPED operation
class OverlappedOperationIO: public OverlappedOperationBase
{
protected:
  /// overlapped operation
  OVERLAPPED _overlapped;
  /// file handle
  HANDLE _fileHandle;

public:
  OverlappedOperationIO();
  virtual ~OverlappedOperationIO();

  /// regular processing
  virtual HRESULT Process();
  /// processed when the operation succeeded
  virtual void OnSucceeded() {}
  /// processed when the operation failed
  virtual void OnFailed() {}

  USE_FAST_ALLOCATOR
};

/// The encapsulation of XEnumerate operation
class EnumerateOperation : public RefCount
{
protected:
  /// handle of the enumeration
  HANDLE _handle;
  /// overlapped operation processing the enumeration
  XOVERLAPPED _overlapped;
  /// buffer receiving the results of single XEnumerate call
  Buffer<BYTE> _buffer;

public:
  EnumerateOperation();
  ~EnumerateOperation();

  /// regular processing
  HRESULT Process();

  virtual void HandleResults(DWORD count) = 0;

  USE_FAST_ALLOCATOR

protected:
  // implementation
  /// Call XEnumerate with the correct arguments
  HRESULT StartEnumeration();
};

#include <Es/Memory/debugNew.hpp>

#ifdef _XBOX
/// Type of save, used for thumbnail selection
enum SaveType
{
  STSaveSingleMission,
  STSaveMPMission,
  STUserMissionSP,
  STUserMissionMP
};
#endif // defined _XBOX

/// Encapsulation for Xbox 360 save game system
class SaveSystem
{
public:
  /// type of save errors
  enum SaveErrorType
  {
    SENone,
    SESaveUser,
    SESaveAuto,
    SESaveContinue,
    SEUserMission,
    SESaveUser2,
    SESaveUser3,
    SESaveUser4,
    SESaveUser5,
    SESaveUser6,
  };

  /// type of request to return to main menu
  enum ReturnRequest
  {
    RRNone, //< no request
    RRSignInChanged, //< sign in changed
    RRLiveDisconnected, //< user was disconnected from Live
    RRCableDisconnected //< network cable was disconnected and user chose the option to disconnect
  };

#ifdef _XBOX
  /// value for privileges
  enum PrivilegeValue
  {
    PVBlocked, //< privilege is blocked
    PVFriendsOnly, //< privilege is allowed for friends only
    PVAllowed //< privilege is allowed
  };

  /// information about user
  class UserInfo
  {
  public:
    UserInfo();

    /// updates information based on given user index
    void Update(int _userIndex);

    /// resets values
    void Reset();

    /// signin info about user
    XUSER_SIGNIN_INFO _signinInfo;

    /// online playing privilege
    PrivilegeValue _onlinePlayPrivilege;
    /// communication privilege
    PrivilegeValue _communicationPrivilege;
    /// video communication privilege
    PrivilegeValue _videoCommunicationPrivilege;
    /// profile viewing privilege
    PrivilegeValue _viewProfilePrivilege;
    /// presence visibility privilege
    PrivilegeValue _presencePrivilege;
    /// user created content view privilege
    PrivilegeValue _userContentViewPrivilege;
    /// content purchase privilege
    PrivilegeValue _contentPurchasePrivilege;
  };
#endif

protected:
  /// storage removal dialog state
  enum StorageRemovalDialogState
  {
    SRNone, //< not shown yet
    SRShown, //< is shown
    SRDeclined, //< storage change was declined    
  };

  /// the receiver of notifications (in XNOTIFY_SYSTEM area)
  HANDLE _listener;
  /// the System UI is shown
  bool _uiShown;

  /// active player signs in / out
  bool _checkInChanged;
  /// there was some change in sign in of active player or inactive player
  bool _someSignInChanged;
  /// storage device changed
  bool _storageChanged;
  /// storage device was removed
  bool _storageRemoved;
  /// if current user has lost connection to live (only set to true, when CURRENT user was connected to Live and lost connection)
  bool _liveConnectionLost;  
  /// flag determining if the user have to be on Live in this stage of game (used for detection if disconnection from Live would return the player to main menu)
  bool _liveConnectionNeeded;
  /// storage removal dialog state
  StorageRemovalDialogState _storageRemovalDialogState;
  /// if there is unhandled save error
  bool _saveError;
  /// request to return to main menu
  ReturnRequest _returnToMainMenuRequest;


#ifdef _XBOX

  /// saves has been changed
  bool _savesChanged;
  /// last file error
  DWORD _lastFileError;

  /**
  SETTINGS stuff 
  User settings data are divided into 2 parts
  - 1. part is written to title specific area of user profile (size of data limited to 3 x 1KB = 3KB)
  - 2. part is written to user save (on storage device)
  */

  /// save of user settings to the Xbox profile is enabled 
  bool _saveSettingsToProfileEnabled;
  /// data in user settings cache hasn't been stored in to the profile yet 
  bool _saveSettingsToProfileCacheDirty;
  /// the user settings cache from Xbox profile is valid
  bool _settingsFromProfileValid;
  /// cache of user settings from Xbox profile
  Buffer<BYTE> _settingsFromProfile;

  /// save of user settings to user save is enabled 
  bool _saveSettingsToSaveEnabled;
  /// data in user settings cache hasn't been stored in to the save yet
  bool _saveSettingsToSaveCacheDirty;
  /// the user settings cache from save is valid (something is there, not necessarily read from storage device)
  bool _settingsFromSaveValid;
  /// the user settings cache from save is valid and we tried to read it from storage device
  bool _settingsFromSaveReadFromSaveTried;
  /// if there is a request to read settings from save
  bool _settingsFromSaveReadRequest;
  /// cache of user settings from save
  Buffer<BYTE> _settingsFromSave;


  /// the cache of Xbox Guide settings is valid
  bool _guideSettingsValid;
  /// cache of Xbox Guide settings
  Buffer<BYTE> _guideSettings;

  /// the cache of Game Defaults settings is valid
  bool _gameDefaultsValid;
  /// cache of Game Defaults settings
  Buffer<BYTE> _gameDefaults;
#endif // defined _XBOX

  /// pending invitation present
  bool _invited;
  /// the user confirmed (progress lost) he want to join session
  bool _inviteConfirmed;
  /// id of ivited user on local console
  int _invitedUserId;
  /// pending invitation info
  XINVITE_INFO _inviteInfo;

#ifdef _XBOX
  /// cache for weapons config (replacement of z:\weapons.cfg)
  Buffer<BYTE> _weaponsConfig;

  /** 
    The index of the currently used profile.
    It is possible that nobody is signed in to this profile index.
  */
  int _userIndex;
  /**    
    if the currently used profile is signed in (live or offline).
    We need this variable, because _userIndex can be valid, but
    nobody may be signed to this profile - so _user can have invalid data
  */
  bool _userSignedIn;
  /// the info about the active user
  UserInfo _userInfo;

  /// information about all possible local players on the console
  AutoArray<UserInfo> _userInfos;

  /// if the _lastContentData is valid
  bool _lastContentDataLoadGameValid;
  /// last content data used when opening saved game
  XCONTENT_DATA _lastContentDataLoadGame;

  /**
  If the currently signed in user owns the last loaded save.
  This value is set to true by default. If some save game is loaded, then we check if the user owns this saved game.
  We set this value to false only if the save game is loaded, user is signed in and user does not own this saved game.
  We have to reset this value to default (true) when user starts to play new game/new mission/new campaign.
  */
  bool _userOwnsSave;

  /// selected device for saves
  XCONTENTDEVICEID _deviceId;
  /// the list of saves is up to date
  bool _savesValid;
  /// list of saves on the selected device
  AutoArray<SaveDescription> _saves;

  /// list of marketplace packages
  AutoArray<XCONTENT_DATA> _packages;

  // list of unlocked achievements
  FindArray<int>_unlockedAchievements;
#else
  /// temporary storage for the info about the signed in user
  XUSER_SIGNIN_INFO _user;
#endif // !defined _XBOX

  /// list of overlapped operations program need not to wait for
  RefArray<OverlappedOperation> _overlappedOperations;

  /// list of enumerations program need not to wait for
  RefArray<EnumerateOperation> _enumerateOperations;

#ifdef _XBOX
  /// The enumeration of save files (need to be handled separately because of result handling)
  Ref<EnumerateOperation> _updateSaves;

  /// The read profile settings operations (separated from _overlappedOperations to cancel them when settings changed)
  Ref<OverlappedOperation> _readSettingsFromProfile;
  /// The read profile settings operations (separated from _overlappedOperations to cancel them when settings changed)
  Ref<OverlappedOperation> _readGuideSettings;
  /// The read profile settings operations (separated from _overlappedOperations to cancel them when settings changed)
  Ref<OverlappedOperation> _readGameDefaults;

  /// Overlapped operation for reading or writing settings from save - creating content
  Ref<OverlappedOperation> _settingsFromSaveContentCreate;
  /// Overlapped operation for reading settings from save - reading file
  Ref<OverlappedOperationIO> _settingsFromSaveReadFile;
  /// Overlapped operation for writing settings to save - writing file
  Ref<OverlappedOperationIO> _settingsFromSaveWriteFile;
  /// Overlapped operation for reading or writing settings from save - closing content
  Ref<OverlappedOperation> _settingsFromSaveContentClose;
#endif // defined _XBOX

public:
  SaveSystem();
  ~SaveSystem();

  /// Single simulation step
  void Simulate();

#ifdef _XBOX
  /// gets the index of the currently used profile
  int GetUserIndex() const {return _userIndex;}
  /** 
    sets the index of the currently used profile, returns if succeeded
    \param index valid user index (not -1 - use ResetUserIndex() for this purpose
  */
  bool SetUserIndex(int index);
  /// sets _userIndex to -1 and resets everything associated with user index
  void ResetUserIndex();
  /// if the currently used profile is signed in (live or offline)
  bool IsUserSignedIn() const {return IsValidUserIndex(_userIndex) && _userSignedIn;}
  /// returns if given index is valid user index
  static bool IsValidUserIndex(int index);
  /// returns if user has privilege to play online
  bool IsOnlinePlayAvailable() const {return IsUserSignedIn() && PVAllowed ==_userInfos[_userIndex]._onlinePlayPrivilege;}

  /// the signin info about the signed in user (or NULL)
  const XUSER_SIGNIN_INFO *GetUserInfo() const
  {
    return IsUserSignedIn() ? &_userInfo._signinInfo : NULL;
  }

  /// returns the user info about user on given index (or NULL)
  const UserInfo *GetUserInfo(int userIndex) const
  {
    return IsValidUserIndex(userIndex) ? &(_userInfos[userIndex]) : NULL;
  }

  /// updates information about users signed in to the console
  void UpdateUserInfos();
#else
  int GetUserIndex() const {return 0;}
  const XUSER_SIGNIN_INFO *GetUserInfo() const;
#endif

  /// The System UI is shown
  bool IsUIShown() const {return _uiShown;}

  /// active player signs in / out
  bool IsCheckInChanged() const {return _checkInChanged;}
  /// active player sign in change was handled
  void CheckInHandled() {_checkInChanged = false;}

  /// if there was some change in sign in of active player or inactive player
  bool IsSomeSignInChanged() const {return _someSignInChanged;}
  /// change in sign in of active player or inactive player was handled
  void SignInChangeHandled() {_someSignInChanged = false;}

  /// actual storage device became unavailable
  bool IsStorageRemoved() const {return _storageRemoved;}
  /// storage device removal was handled
  void StorageRemovedHandled() {_storageRemoved = false;}

#ifdef _XBOX
  /// storage device changed (removed or selected other storage device)
  bool IsStorageChanged() const {return _storageChanged;}
  /// storage device change was handled
  void StorageChangeHandled() {_storageChanged = false;}
#endif

  /// saves has been successfully updated
  bool IsSavesChanged() const {return _savesChanged;}
  /// saves change has been handled
  void SavesChangeHandled() {_savesChanged = false;}

  /// if current user has lost connection to live (only set to true, when CURRENT user was connected to Live and lost connection)
  bool IsLiveConnectionLost() const {return _liveConnectionLost;}  
  /// the connection lost to Live was handled
  void LiveConnectionLostHandled() {_liveConnectionLost = false;}

  /// sets storage removal dialog state to SRDeclined
  void StorageRemovalDialogDeclined() {_storageRemovalDialogState = SRDeclined;}
  /// sets storage removal dialog state to SRNone
  void StorageRemovalDialogResetState() {_storageRemovalDialogState = SRNone;}
  /// sets storage removal dialog state to SRShown
  void StorageRemovalDialogShown() {_storageRemovalDialogState = SRShown;}

  bool IsStorageRemovalDialogDeclined() {return SRDeclined == _storageRemovalDialogState;}
  bool IsStorageRemovalDialogShown() {return SRShown == _storageRemovalDialogState;}

  /// gets request to return to main menu (can be none)
  ReturnRequest GetReturnToMainMenuRequest() const {return _returnToMainMenuRequest;}
  /// sets request to return to main menu (can be none)
  void SetReturnToMainMenuRequest(ReturnRequest request) {_returnToMainMenuRequest = request;}
  /// resets request to return to main menu (sets to RRNone)
  void ResetReturnToMainMenuRequest() {SetReturnToMainMenuRequest(RRNone);}

  /// returns flag determining if the user have to be on Live in this stage of game (used for detection if disconnection from Live would return the player to main menu)
  bool IsLiveConnectionNeeded() const {return _liveConnectionNeeded;}
  /// sets flag determining if the user have to be on Live in this stage of game (used for detection if disconnection from Live would return the player to main menu)
  void SetLiveConnectionNeeded(bool value) {_liveConnectionNeeded = value;}



#ifdef _XBOX

  /// returns error code of the last file operation
  DWORD GetLastFileError() const {return _lastFileError;}
  /// set last save error manually
  void SetLastFileError(DWORD errorCode) {_lastFileError = errorCode;}

  /// checks if user owns last loaded game (_lastContentDataLoadGame)
  void CheckUserOwnsSave();
  /// sets _userOwnsSave to given value 
  void SetUserOwnsSave(bool value) {_userOwnsSave = value;}
  /// reset _userOwnsSave to default value (true)
  void ResetUserOwnsSave() {_userOwnsSave = true;}

  /// sets if writing of user settings to Xbox profile is enabled or it's saving only to the cache
  void SetSaveSettingsToProfileEnabled(bool enabled) {_saveSettingsToProfileEnabled = enabled;}
  /// sets if writing of user settings to user save is enabled or it's saving only to the cache
  void SetSaveSettingsToSaveEnabled(bool enabled) {_saveSettingsToSaveEnabled = enabled;}

  /// returns if there is already some content package opened
  bool IsSomeSaveOpened() const {return XSaveGameImpl::_instances > 0;}
#endif

  /// removes storage device
  void RemoveStorageDevice();

  /// Check if the player is signed in
  bool CheckSignIn(bool online, bool mustHaveMPPrivilege) const;
  /// Force the sign in UI
  void SignIn(bool online);

  /// Accepted invitation is pending
  bool IsInvited() const {return _invited;}
  /// Accepted invitation was handled
  void InvitationHandled() {_invited = false;}
  /// Invitation was confirmed
  bool IsInvitationConfirmed() const {return _inviteConfirmed;}
  /// Accepted invitation was handled
  void ConfirmInvitation() {_inviteConfirmed = true;}
  /// Accepted invitation info access
  const XINVITE_INFO &GetInviteInfo() const {return _inviteInfo;}
  /// Gets invited user id on local console
  int GetInvitedUserId() const {return _invitedUserId;}

#ifdef _XBOX
  /// returns if storage device is available for saving
  bool IsStorageAvailable() const { return IsUserSignedIn() && _deviceId != XCONTENTDEVICE_ANY;}
  /// Check if device is selected (if not selected, then selection UI is displayed)
  bool CheckDevice(DWORD bytesRequested);

  /// Check if list of game saves is up-to-date
  bool CheckSaves();
  /// Force refresh of the list of game saves
  void SavesChanged();
  /// Count of enumerated saves
  int NSaves() const {return _saves.Size();}
  /// Return the enumerated save description
  const SaveDescription &GetSave(int i) const {return _saves[i];}

  /// Create a new save slot
  XSaveGame CreateSave(RString fileName, RString displayName, SaveType saveType);
  /// Open an existing save slot if exist, otherwise create a new one
  XSaveGame OpenOrCreateSave(RString fileName, RString displayName, SaveType saveType, DWORD *errorCode = NULL, bool noErrorMsg = false);
  /// Open an existing save slot
  /**
  when error is given, the function will not report the error itself and return the error code there
  */
  XSaveGame OpenSave(RString fileName, DWORD *errorCode = NULL, bool noErrorMsg = false);
  /// Destroy an existing save slot
  bool DeleteSave(RString fileName);

  /** profile settings need to be updated, 
     \param readProfileSettings load settings from Xbox profile
  */
  void SettingsChanged(bool readSettingsFromProfile);

  /// resets all flags for settings from save (need to be called when user index changes)
  void ResetSettingsFromSave();

  /// returns if there is some overlapped operation working with settings from save in progress (read or write)
  bool IsSettingsFromSaveInProgress() const;

  /// read the settings from profile for the signed in player
  bool ReadSettingsFromProfile(QIStrStream &in);
  /// write the settings to the profile for the signed in player
  bool WriteSettingsToProfile(QOStrStream &out);

  /// read the settings from save for the signed in player
  bool ReadSettingsFromSave(QIStrStream &in);
  /// write the settings to save for the signed in player
  bool WriteSettingsToSave(QOStrStream &out);

  /// Get the value of Xbox Guide settings, -1 if not valid
  int GetGuideSettings(DWORD id);
  /// Get the value of Game Defaults settings, -1 if not valid
  int GetGameDefaults(DWORD id);

  /// read the content of weapon config
  bool ReadWeaponConfig(QIStrStream &in);
  /// write the content of weapon config
  void WriteWeaponConfig(QOStrStream &out);
  /// clean up the content of weapon config
  void ClearWeaponConfig();
#endif // defined _XBOX

  /// register the overlapped operation for processing
  void StartOperation(OverlappedOperation *operation) {_overlappedOperations.Add(operation);}
  /// register the enumeration operation for processing
  void StartOperation(EnumerateOperation *operation) {_enumerateOperations.Add(operation);}
  
  /// set the game context (matchmaking, rich presence)
  bool SetContext(DWORD contextId, DWORD contextValue);

  /// set the game property (matchmaking, rich presence)
  bool SetProperty(DWORD propertyId, int propertyValue);

  /// change the number of slots for the session
  bool UpdateSession(HANDLE sessionHandle, DWORD flags, int maxPlayers, int privateSlots);

#ifdef _XBOX
  /** 
    Process device selection
    \param forceShowUI if we should show selection UI even if only 1 device is available
  */
  bool SelectDevice(DWORD bytesRequested, bool forceShowUI = false);
  /// enumerate and open all downloaded content
  bool InitMarketplace();
  /// close all downloaded content
  void DoneMarketplace();
  /// process the given action for each downloaded content
  template <typename Function>
  void ForEachPackage(Function f) const
  {
    char buffer[13]; // symbolic name can be 1...12 chars length
    for (int i=0; i<_packages.Size(); i++)
    {
      sprintf(buffer, MARKETPLACE_DRIVE_FORMAT, i);
      f(buffer);
    }
  }

  /// handler called when the settings read operation succeeds
  void OnSettingsFromProfileRead(XUSER_READ_PROFILE_SETTING_RESULT *header);
  /// handler called when the settings read operation succeeds
  void OnGuideSettingsRead(XUSER_READ_PROFILE_SETTING_RESULT *header);
  /// handler called when the settings read operation succeeds
  void OnGameDefaultsRead(XUSER_READ_PROFILE_SETTING_RESULT *header);

  /** handler called when the creation of content while reading or writing settings from save succeeds
  \param isWriting if we are writing content
  */
  void OnSettingsFromSaveContentCreated(bool isWriting);

  /// handler called when the settings from save read operation succeeds
  void OnSettingsFromSaveRead(Buffer<BYTE>& data);

  /// closes content after reading or writing settings to save
  void StartSettingsFromSaveCloseContent();


  /** unlock specified achievement 
  \param achievement number of achievement (taken from spa.h)
  \return returns true iff achievement was really added (if user already had this achievement, false is returned!)
  */
  bool UnlockAchievement(int achievement);

#endif // defined _XBOX

protected:
#ifdef _XBOX
  /** 
    Checks if something about currently used profile changed 
    (called when notification XN_SYS_SIGNINCHANGED arrived 
  */
  bool CheckActiveSignInChanged();
  /// checks if current storage device changed (became unavailable)
  bool CheckStorageChanged();
  /// changes storage device to given device id
  void SetDeviceId(XCONTENTDEVICEID newDeviceId);
  /// resets device id
  void ResetDeviceId();

  /// read the settings from profile if the user is not signed in
  bool ReadSettingsFromProfileNoUser(QIStrStream &in);
  /// write the settings to profile if the user is not signed in
  bool WriteSettingsToProfileNoUser(QOStrStream &out);
  /// read the settings from save if the user is not signed in or no storage device is selected
  bool ReadSettingsFromSaveNoUser(QIStrStream &in);
  /// write the settings to save if the user is not signed in or no storage device is selected
  bool WriteSettingsToSaveNoUser(QOStrStream &out);
  /// write settings from cache into the Xbox profile
  bool WriteSettingsToProfileInternal();
  /// write settings from cache into the Xbox save
  bool WriteSettingsToSaveInternal();


  /// Updates information about currently used profile
  void UpdateActiveUserInfo();
  /// Process refresh of the list of game saves
  bool UpdateSaves();
  /// Process refresh of Xbox Guide settings
  bool UpdateGuideSettings();
  /// Process refresh of Game Defaults settings
  bool UpdateGameDefaults();
  /// Create and start the save update operation
  HRESULT StartUpdateSaves();
  /// Create and start the profile settings read operation
  HRESULT StartSettingsFromProfileRead();
  /// Create and start the profile settings read operation
  HRESULT StartGuideSettingsRead();
  /// Create and start the profile settings read operation
  HRESULT StartGameDefaultsRead();

  /// Create and start the settings from save read operation
  HRESULT StartSettingsFromSaveRead();

  /// Add thumbnail to the save based on the save type
  HRESULT SetThumbnail(const XCONTENT_DATA *saveDesc, SaveType saveType);

  /// returns the displayName of the save in _saves
  RString GetSaveDisplayName(const RString &saveFile);

  /// load achievements from current profile
  bool LoadProfileAchievements();
#endif // defined _XBOX
};

#ifdef _XBOX
TypeIsMovable(SaveSystem::UserInfo);
#endif

extern SaveSystem GSaveSystem;

#elif defined _XBOX

#include <El/Enum/dynEnum.hpp>

#define SAVE_TYPE_ENUM(type,prefix,XX) \
  XX(type, prefix, GameSave) \
  XX(type, prefix, Settings) \
  XX(type, prefix, Mission) \
  XX(type, prefix, MPMission)

#ifndef DECL_ENUM_SAVE_TYPE
#define DECL_ENUM_SAVE_TYPE
DECL_ENUM(SaveType)
#endif
DECLARE_ENUM(SaveType, ST, SAVE_TYPE_ENUM);

struct SaveHeaderAttribute
{
  RString name;
  RString value;
  SaveHeaderAttribute(){}
  SaveHeaderAttribute(RString n, RString v)
  {
    name = n; value = v;
  }
};
TypeIsMovableZeroed(SaveHeaderAttribute)

class SaveHeaderAttributes : public AutoArray<SaveHeaderAttribute>
{
  typedef AutoArray<SaveHeaderAttribute> base;

public:
  int Add(RString name, RString value);
  bool Find(RString name, RString &value) const;

  // equal only if attributes are in the same order
  bool operator ==(const SaveHeaderAttributes &with) const;
  bool operator !=(const SaveHeaderAttributes &with) const {return !operator ==(with);}
};

struct SaveHeader
{
  RString name;
  RString dir;
  SaveHeaderAttributes attributes;
};
TypeIsMovable(SaveHeader)

void FindSaves(AutoArray<SaveHeader> &list, SaveHeaderAttributes &filter);
void FindSaves(AutoArray<SaveHeader> &list, SaveHeaderAttributes &filter, bool caseSensitive, bool corrupted);
RString GetSaveDir(SaveHeaderAttributes &filter);
RString CreateSave(SaveHeaderAttributes &attributes);
bool DeleteSave(RString dir, RString name);
bool DeleteSave(SaveHeaderAttributes &filter);
bool DeleteSave(SaveHeaderAttributes &filter, bool caseSensitive);
void CopyDefaultSaveGameImage(RString dir);

#endif // _XBOX_VER >= 200
#endif // _SAVE_GAME_HPP

