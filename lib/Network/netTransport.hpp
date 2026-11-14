#ifdef _MSC_VER
#pragma once
#endif

/*!
\file
Interface of network transport implementation classes
*/

#ifndef _NET_TRANSPORT_HPP
#define _NET_TRANSPORT_HPP

#include <Es/Strings/rString.hpp>
#include <Es/Containers/array.hpp>
#include <El/Network/netXboxConfig.hpp>
#include <El/ParamFile/paramFileDecl.hpp>
#include <El/Speech/vonPch.hpp>
#include <El/Speech/vonApp.hpp>

#include "winSockImpl.hpp"

struct LocalizedString;

//! Description of multiplayer session
struct SessionInfo
{
	//! URL of host
	RString guid;
#if _ENABLE_STEAM_MATCHMAKING
  // SteamID from SteamMatchmaking
  unsigned __int64 steamID;
  int smQueryPort;
  int smIP;
  int smPort;
#endif
	//! Name of session
	RString name;
	//! Last time session was accessed
	DWORD lastTime;
	//! Host's current version
	int actualVersion;
	//! Host's current build no
	int actualBuildNo;
	//! Host's required version
	int requiredVersion;
  //! Host's required build no
  int requiredBuildNo;
	//! Session is password protected
	bool password;
  //! Session is locked
  bool lock;
	//! Host's version is too old
	bool badActualVersion;
	//! Host's version is too new
	bool badRequiredVersion;
  //! Mod lists differs
  bool badMod;
  //! Additional mods on server
  bool badAdditionalMods;
  //! Missing Signature files or some files patched
  bool badSignatures;
	//! Game type
	RString gameType;
	//! Mission name
	RString mission;
	//! Island
	RString island;
	//! State of game on host
	int serverState;
	//! Ping time (in ms)
	int ping;
	//! Current number of players (in public slots)
	int playersPublic;
	//! Current number of friends
	int friends;
  //! Added in Favorite server list
  mutable bool favorite;
	//! Maximal number of players (public slots)
	int slotsPublic;
	//! Estimated time to mission end (in minutes)
	int timeleft;
	//! Platform (Windows/Linux/Unknown)
	RString platform;
  //! mod list on host
  RString mod;
  //! equal mod list is required by host
  bool equalModRequired;
  //! dedicated server
  bool dedicated;
  //! selected game language id
  int language;
  //! selected mission difficulty
  int difficulty;
  //! country where server is placed
  RString country;
  //! region code where server is placed
  int region;
  //! battle eye 
  int battleEye;
  //! time of day inside mission on server hh:mm
  RString timeOfDay;
  //! disable3rdPerson servers for mode regular/hardcore
  int disable3rdPerson ;  //1 for disabled, ie. hardcore
	//! time multiplier of environment
	float environmentTimeMul;
  //! public/private hive based on instanceID (000000 and 000001 for public))
  int privateHive;  //0 for public, 1 for private 

  int longitude,latitude;
};
TypeIsMovableZeroed(SessionInfo)

/*!
\patch 2.06 Date 6/23/2003 by Jirka
- Added: Testing of -mod compatibility in MP
*/

#define MOD_LENGTH  80

//! Basic information sent in session description as user data
struct MPVersionInfoOld
{
	//! actual application version
	int versionActual;
	//! required application version
	int versionRequired;
	//! name of running mission
	char mission[40];
	//! state of network server
	int serverState;
};

struct MPVersionInfo : public MPVersionInfoOld
{
  //! -mod list
  char mod[MOD_LENGTH];
};

//! Result of connecting client to server
#ifndef DECL_ENUM_CONNECT_RESULT
#define DECL_ENUM_CONNECT_RESULT
DECL_ENUM(ConnectResult)
#endif
DEFINE_ENUM_BEG(ConnectResult)
	//! connect in progress
	CRNone = -1,
	//! connected
	CROK,
	//! bad password
	CRPassword,
	//! incompatible program version
	CRVersion,
	//! other error
	CRError,
	//! session full (maximum number of players allotted for the session has been reached)
	CRSessionFull,
DEFINE_ENUM_END(ConnectResult)

// flags for message
enum NetMsgFlags
{
	NMFNone=0,
	NMFGuaranteed=1,
	NMFHighPriority=2,
	NMFStatsAlreadyDone=4,
  NMFSetCallback=8
};

enum NetTerminationReason
{
	NTRTimeout,
	NTRDisconnected,
	NTRKicked,
	NTRBanned,
	NTRMissingAddon,
  NTRBadCDKey,
  NTRCDKeyInUse,
  NTRSessionLocked,
  NTRBattlEye,
  NTRAuthTimeOut,
	NTROther, // must be last
};

__forceinline NetMsgFlags operator | (NetMsgFlags a, NetMsgFlags b)
{
	return NetMsgFlags((int)a|(int)b);
}

//! Info about message completion
/*!
Used for transfer information from receiving thread to main application thread.
*/
struct SendCompleteInfo
{
	DWORD msgID;
	bool ok;
};
TypeIsSimple(SendCompleteInfo)

//! Info about new player
/*!
Used for transfer information from receiving thread to main application thread.
*/
struct CreatePlayerInfo
{
	int player;
	bool botClient;
	bool privateSlot;
  unsigned long inaddr;
	char name[56];
  char mod[MOD_LENGTH];
};
TypeIsSimple(CreatePlayerInfo)

//! Info about removed player
/*!
Used for transfer information from receiving thread to main application thread.
*/
struct DeletePlayerInfo
{
	int player;
};
TypeIsSimple(DeletePlayerInfo)

//! additional remote host info
struct RemoteHostAddress
{
	RString name;
	RString ip;
	int port;
};
TypeIsMovableZeroed(RemoteHostAddress)

typedef void UserMessageClientCallback(char *buffer, int bufferSize, void *context);
typedef void UserMessageServerCallback(int from, char *buffer, int bufferSize, void *context);
typedef void SendCompleteCallback(DWORD msgID, bool ok, void *context);
typedef void CreatePlayerCallback(int player, bool botClient, bool privateSlot, const char *name, const char *mod, unsigned long inaddr, void *context);
typedef void DeletePlayerCallback(int player, void *context);
typedef bool CreateVoicePlayerCallback(int player, void *context);

//! Interface for class supporting session enumeration
class NetTranspSessionEnum
{
public:
	//! Constructor
	NetTranspSessionEnum() {}
	//! Destructor
	virtual ~NetTranspSessionEnum() {}

	virtual bool Init( int magic =0 ) = 0;

	virtual bool RunningEnumHosts() = 0;
	virtual bool StartEnumHosts(RString ip, int port, AutoArray<RemoteHostAddress> *hosts) = 0;
	virtual void StopEnumHosts() = 0;
  virtual void ClearEnumHosts () = 0;

	virtual int NSessions() = 0;
	virtual void GetSessions(AutoArray<SessionInfo> &sessions) = 0;

	//! Transfer IP address and port into URL address
	virtual RString IPToGUID(RString ip, int port) = 0;
};

struct VoiceMask;

class ParamEntry;
typedef bool CancelNNCallback();

//! Interface for network transport client class
class NetTranspClient
{
public:
	//! Constructor
	NetTranspClient() {}
	//! Destructor
	virtual ~NetTranspClient() {}

	//! Initialization of client
	/*!
	\param address URL address of host
	\param password session password
	\param botClient client is bot client (must be sent to server in connection packet)
	\param playerName name of player (must be sent to server in connection packet)
	\param versionInfo info about actual and required application version
				(must be sent to server in connection packet)
	\param magic application identifier
	\param port
				- on input: expected host IP port (obtained form session enumeration or user input)
				- on output: real session port client was connected to
	\return result of connect
	*/
	virtual ConnectResult Init
	(
		RString address, RString password, bool botClient, int &port,
		RString player, MPVersionInfo &versionInfo, int magic =0, CancelNNCallback *cancelNNCallback=NULL
	) = 0;
	//! Create and initialize DirectPlay voice client
	virtual bool InitVoice(VoiceMask *voiceMask, RawMessageCallback callback, bool isBotClient) = 0;

	virtual Ref<NetMessage> SendMsg(BYTE *buffer, int bufferSize, DWORD &msgID, NetMsgFlags flags, const Ref<NetMessage> &dependOn) = 0;
	virtual void GetSendQueueInfo(int &nMsg, int &nBytes, int &nMsgG, int &nBytesG) = 0;
	virtual bool GetConnectionInfo(int &latencyMS, int &throughputBPS) = 0;
	virtual bool GetConnectionInfoRaw(int &latencyMS, int &throughputBPS)
	{
		return GetConnectionInfo(latencyMS, throughputBPS);
	}

  virtual void SetNetworkParams ( ParamEntryPar cfg ) {}
  
  virtual bool GetLocalAddress( in_addr &addr ) const {return false;}
  virtual bool GetLocalAddress( in_addr &addr, int &port ) const {return false;}
  virtual bool GetDistantAddress (in_addr &addr, int &port) const {return false;}

  virtual bool GetServerAddress ( sockaddr_in &addr ) const {return false;}

  virtual int GetVoicePort() const = 0;
  virtual SOCKET GetVoiceSocket() const = 0;

	virtual float GetLastMsgAge() = 0;
	virtual float GetLastMsgAgeReliable() = 0;
	virtual float GetLastMsgAgeReported() = 0;
	virtual void LastMsgAgeReported() = 0;

	virtual bool IsSessionTerminated() = 0;
  virtual NetTerminationReason GetWhySessionTerminated() = 0;
  virtual RString GetWhySessionTerminatedStr() = 0;

  virtual void SetVoiceTransmition(bool val) = 0;
  virtual void SetVoiceToggleOn(bool val) = 0;
  /// sets threshold for silence analyzer (0.0f - 1.0f)
  virtual void SetVoNRecThreshold(float val) = 0;
  virtual void SetVoNTransmitterFreq(float f) = 0;
  virtual float GetVoNTransmitterFreq() const = 0;
  virtual void SetVoNTransmitterRange(float range) = 0;
  virtual float GetVoNTransmitterRange2() const = 0;

  //! Return true, if given remote player is speaking and voice is audible on local computer
	virtual bool IsVoicePlaying(int player) = 0;
  //! Return true, if player on local computer is speaking and voice is recording for Voice Over Net
	virtual bool IsVoiceRecording() = 0;
  virtual int CheckVoiceChannel(int dpnid, bool &audible2D, bool &audible3D) const = 0;
  virtual void IsVoice2D3DPlaying(int dpnid, bool &audible2D, bool &audible3D) const = 0;
#if _ENABLE_CHEATS
  virtual void VoNSay(int dpnid, int channel, int frequency, int seconds) = 0;
#endif
  virtual void VoiceCapture ( bool on, int quality )
  {}
  virtual void ChangeVoiceMask ( VoiceMask *voiceMask )
  {}
  virtual void SetVoNPosition(int dpnid, Vector3Par pos, Vector3Par speed) = 0;
  virtual void SetVoNObstruction(int dpnid, float obstruction, float occlusion, float deltaT) = 0;
  virtual void UpdateVoNVolumeAndAccomodation(int dpnid, float volume, float accomodation) = 0;
  virtual void SetVoNPlayerPosition(Vector3Par pos, Vector3Par speed) = 0;
  virtual void AdvanceAllVoNSounds(float deltaT, bool paused, bool quiet) = 0;

	virtual void ProcessUserMessages(UserMessageClientCallback *callback, void *context) = 0;
	virtual void RemoveUserMessages() = 0;
	virtual void ProcessSendComplete(SendCompleteCallback *callback, void *context) = 0;
	virtual void RemoveSendComplete() = 0;

    virtual RString GetStatistics ()
    { return RString(); }

    virtual unsigned FreeMemory ()
    { return 0; }

};

//! Interface for network transport server class
class NetTranspServer
{
public:
	//! Constructor
	NetTranspServer() {}
	//! Destructor
	virtual ~NetTranspServer() {}

	//! Initialization of server
	/*!
	\param port recommended IP port
	\param password session password (information if password is required must be included in enumeration response)
	\param hostname host name - must be included in session name sent in enumeration response, if empty, use IP address:port instead
	\param maxPlayers maximal number of connected players (must be incuded in enumeration response)
	\param sessionNameInit session name used if host address cannot be obtained
	\param sessionNameFormat format of session name (printf syntax) - playerName and hostName are sent as parameters
	\param playerName name of hosting player (used in session name)
	\param versionInfo info about actual and required application version
				(used to decide if client can connect)
	\param magic application identifier
	*/
	virtual bool Init
	(
    RString name, RString password, int maxPlayers, int port,
		int language, int difficulty,
		MPVersionInfo &versionInfo, bool equalModRequired, int magic =0
	) = 0;
  //! Update maximal number of players allowed
  /*!
  \param maxPlayers maximal number of connected players (must be included in enumeration response)
  */
  virtual void UpdateMaxPlayers(int maxPlayers) = 0;
  //! When Session is changed to Locked we need to broadcast it
  virtual void UpdateLockedOrPassworded(bool lock, bool passworded) = 0;
	//! Create and initialize DirectPlay voice server
	virtual bool InitVoice() = 0;

	virtual RString GetSessionName() = 0;
	//! Return real local port of session
	virtual int GetSessionPort() = 0;

	virtual Ref<NetMessage> SendMsg(int to, BYTE *buffer, int bufferSize, DWORD &msgID, NetMsgFlags flags, const Ref<NetMessage> &dependOn) = 0;
	virtual void CancelAllMessages() = 0;
	virtual void GetSendQueueInfo(int to, int &nMsg, int &nBytes, int &nMsgG, int &nBytesG) = 0;
	virtual bool GetConnectionInfo(int to, int &latencyMS, int &throughputBPS) = 0;
	virtual bool GetConnectionInfoRaw(int to, int &latencyMS, int &throughputBPS)
	{
		return GetConnectionInfo(to, latencyMS, throughputBPS);
	}
	virtual void GetConnectionLimits(int &maxBandwidthPerClient)
	{
	  maxBandwidthPerClient = INT_MAX;
	}
  virtual void SetConnectionLimits(int maxBandwidthPerClient)
  {
  }
  virtual void SetNetworkParams ( ParamEntryPar cfg )
  {}
	virtual float GetLastMsgAgeReliable(int player) = 0;

	virtual void UpdateSessionDescription(
	  int state, RString gameType, const LocalizedString &mission, RString island,
    int timeleft, int difficulty
	) = 0;
	virtual void KickOff(int player, NetTerminationReason reason, const char *reasonStr=NULL) = 0;
	//! Retrieves URL of server
	virtual bool GetURL(char *address, DWORD addressLen) = 0;

  virtual bool GetServerAddress(sockaddr_in &address) = 0;
  virtual bool GetClientAddress(int client, sockaddr_in &address) = 0;

	virtual void GetTransmitTargets(int from, AutoArray<int, MemAllocSA> &to) = 0;
	virtual void SetTransmitTargets(int from, AutoArray<int, MemAllocSA> &to) = 0;
	virtual void UpdateTransmitTargets() {}
  virtual void GetVoNTopologyDiag(QOStrStream &out, Array<ConvPair> &convTable) {}

	virtual void ProcessUserMessages(UserMessageServerCallback *callback, void *context) = 0;
	virtual void RemoveUserMessages() = 0;
	virtual void ProcessSendComplete(SendCompleteCallback *callback, void *context) = 0;
	virtual void RemoveSendComplete() = 0;
	virtual void ProcessPlayers(CreatePlayerCallback *callbackCreate, DeletePlayerCallback *callbackDelete, void *context) = 0;
	virtual void RemovePlayers() = 0;
	virtual void ProcessVoicePlayers(CreateVoicePlayerCallback *callback, void *context) = 0;
	virtual void RemoveVoicePlayers() = 0;

    virtual RString GetStatistics (int player)
    { return RString(); }

    virtual unsigned FreeMemory ()
    { return 0; }

};

struct TransmitTarget;

/// set voice receivers for peer-to-peer voice
void VoiceTransmitTargets(const TransmitTarget *targets, int nAddr);

/// list all players for peer-to-peer voice
void VoiceAllTargets(const TransmitTarget *targets, int nAddr);

/// when port address was changed in VoN layer, change it also in PlayerIdentity
void ProcessPeerChanged(PeerChangedCallback proc, void *context);

/// get diagnostics string for given dpid from transmit targets
RString GetVoiceTargetDiagnostics(int dpid);

/// get diagnostics string of active transmit target "cliques"
void GetVoiceCliquesDiagnostics(QOStrStream &out, Array<ConvPair> &convTable);

/// start/stop VoN KeepAlive system (sending probes,response,keepAlive packets)
void VoiceStartKeepAlive(bool start, int dpid);

/// set actual chat channel
void VoiceSetChatChannel(VoNChatChannel chatChan);

/// fill in the array by dpnid of all players within the maxDist distance using position from their VoNSoundBufferOAL
void WhatUnitsVoNDirect(AutoArray<int> &units, Vector3Par pos, float maxDist);

//! Create Net implementation of NetTranspSessionEnum class
NetTranspSessionEnum *CreateNetSessionEnum( ParamEntryPar cfg );
//! Create Net implementation of NetTranspClient class
NetTranspClient *CreateNetClient( ParamEntryPar cfg );
//! Create Net implementation of NetTranspServer class
NetTranspServer *CreateNetServer( ParamEntryPar cfg );

//! Nat Negotiation critical section
void enterNN();
void leaveNN();

#endif
