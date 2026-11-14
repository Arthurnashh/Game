/**
  @file   netTransportXbox.cpp
  @brief  NetTransport implementation using Net* library (for Xbox secure sockets).

  Copyright &copy; 2001-2004 by BIStudio (www.bistudio.com)
  @author PE
  @date   14.6.2004
*/

#include "../wpch.hpp"
#include "netTransport.hpp"
#include "El/ParamFile/paramFile.hpp"
#include "El/Stringtable/stringtable.hpp"
#include "../progress.hpp"

#if _ENABLE_MP

#undef  PEER2PEER

#include "../memHeap.hpp"
#include "El/Network/netpch.hpp"

#if _XBOX_SECURE && _XBOX_VDP

#include "El/Network/peerfactoryxbox.hpp"
#include "El/Network/netpeerxbox.hpp"
#include "El/Network/netchannel.hpp"

#if VOICE_OVER_NET
#  include "El/Speech/vonPch.hpp"
#  include "El/Speech/vonMemory.hpp"
#  include "voiceXbox.hpp"
#endif

#define GET_SERVER_PEER   getGamePeer
#define GET_CLIENT_PEER   getGamePeer
#define GET_ENUM_PEER     getBcastPeer
#define GET_P2P_PEER      getP2pPeer

//---------------------------------------------------------------------------
//  forward declarations:

class NetSessionEnum;
class NetClient;
class NetServer;
class NetPeerToPeer;

void decodeURLAddress ( RString address, RString &ip, int &port );
void LoadNetworkParams ( NetworkParams &data, ParamEntryPar cfg );
Ref<NetMessage> mergeMessageList ( NetMessage *msg );
NetPool *getPool ();

//---------------------------------------------------------------------------
//  interface:

#ifndef DPNID_ALL_PLAYERS_GROUP
#  define DPNID_ALL_PLAYERS_GROUP   0
#endif

#define MAX_SESSIONS             16
#define LEN_SESSION_NAME         16
#define LEN_GAMETYPE_NAME         8
#define LEN_MISSION_NAME         40
#define LEN_ISLAND_NAME          16

#define UNIQUE_TO_TRY            12

#define RESERVED_IDS              8

#define LEN_PLAYER_NAME          40
#define LEN_PASSWORD_NAME        40

#if !_SUPER_RELEASE
#  undef DEDICATED_STAT_LOG
#endif

#if _ENABLE_DEDICATED_SERVER
  bool IsDedicatedServer();
  int ConsoleF( const char *format, ... );
#else
#  undef DEDICATED_STAT_LOG
#endif

#ifdef DEDICATED_STAT_LOG
#  define CONSOLE_LOG_INTERVAL 2000000
#endif

#ifdef NET_LOG_TRANSP_STAT
#  define STAT_LOG_INTERVAL     80000
#endif

/// Maximum enumeration message age (in milliseconds).
#define MAX_ENUM_AGE          15000
/// Minimum enumeration retry time (in milliseconds).
#define MIN_ENUM_RETRY         4000
/// Maximum time to wait for AckPlayer response (in milliseconds).
#define ACK_PLAYER_TIMEOUT     8000
/// Re-send time for MAGIC_CREATE_PLAYER and MAGIC_RECONNECT_PLAYER (in milliseconds).
#define CREATE_PLAYER_RESEND   2000
/// Time interval to check network response (NetClient::Init(), in milliseconds).
#define NET_CHECK_WAIT          100
/// Time to wait in network destructors (for last message departure, in milliseconds).
#define DESTRUCT_WAIT           500
/// Time to wait for the higher-layer to process player-delete operation (in milliseconds).
#define DESTROY_WAIT            100
/// Send-timeout for common messages (in micro-seconds).
#define SEND_TIMEOUT            800000

/// Tag for messages starting with 32-bit "magic" number.
#define MSG_MAGIC_FLAG          0x0001

/// Define this symbol if both legacy and new enumeration responses will be sent.
//#define ENUM_LEGACY

  // magic numbers (sent in little-endian format /Intel/):

/// Enumeration request (client:broadcast -> server:broadcast).
#define MAGIC_ENUM_REQUEST      0xeee191ae
/// Enumeration response (server:broadcast -> client:broadcast).
#define MAGIC_ENUM_RESPONSE     0xfff1e8ac
/// Enumeration response - legacy version (server:broadcast -> client:broadcast).
#define MAGIC_ENUM_RESPONSE_LEGACY 0xfff4fe12
/// CreatePlayer request (client:normal -> server:broadcast). Not VIM. Duplicate messages should be ignored.
#define MAGIC_CREATE_PLAYER     0xccca1e12
/// AckPlayer response (server:both -> client:normal). I need some acknowledgement for broadcast sender (in far future).
#define MAGIC_ACK_PLAYER        0xaaa51a7e
/// ReconnectPlayer request (client:normal -> server:broadcast).
#define MAGIC_RECONNECT_PLAYER  0x111044ec
/// Terminate the session (server:normal -> clients:normal). Not VIM.
#define MAGIC_TERMINATE_SESSION 0x777814a1
/// DestroyPlayer message (client:normal -> server:normal). Not VIM.
#define MAGIC_DESTROY_PLAYER    0xddd15072
/// Peer-to-peer request (p2p:client -> p2p:server)
#define MAGIC_P2P_REQUEST       0x000d184a
/// Peer-to-peer answer (p2p:server -> p2p:client)
#define MAGIC_P2P_ANSWER        0x222a78b3
/// Negative answer to p2p request (fatal error, server is not running..)
#define MAGIC_P2P_NEGATIVE      0x3337e0a1

#define XBOX_GAME_PORT          1000
#define XBOX_BCAST_PORT         1001
#define XBOX_P2P_PORT           1002 // used for voice

#define NN_NUM_PORTS_TO_TRY        3

#pragma pack (push,netPackets,1)

/// Universal packet starting with 32-bit magic number.
struct MagicPacket
{

  /// Magic number (MAGIC_*).
  unsigned32 magic;

} PACKED;

/// Enumeration request (will be sent through network).
struct EnumPacket : public MagicPacket
{

  /// Application-specific magic number.
  int32 magicApplication;

  /// source Xbox identification.
  unsigned32 source;

} PACKED;

/// Session description packet (will be sent through network).
struct SessionPacket : public MagicPacket
{

  /// Name of session.
  char name[LEN_SESSION_NAME];

  /// Current version of application on host.
  int32 actualVersion;

  /// Required version of application on host.
  int32 requiredVersion;

  /// Game type of running mission on host.
  char gameType[LEN_GAMETYPE_NAME];

  /// Localization type for the name of the mission
  char missionLocalizedType;
  /// Name of running mission on host.
  char missionId[LEN_MISSION_NAME-1];

  /// Island where running mission on host.
  char island[LEN_ISLAND_NAME];

  /// State of game on host.
  int32 serverState;

  /// Maximal number of players + 2 (or 0 if not restricted)
  int32 maxPlayers;

  /// Password is required.
  int16 password;

  /// Receiving port.
  int16 port;

  /// Actual number of players.
  int32 numPlayers;

  /// Serial number of enumeration request.
  MsgSerial request;

  /// Maximal number of private players (should be <= maxPlayers).
  int32 maxPrivate;

  /// Actual number of private players (should be <= numPlayers).
  int32 numPrivate;

  /// Xbox title address.
  XNADDR xaddr;

  /// Session ID.
  XNKID kid;

  /// Key for session ID.
  XNKEY key;

  /// Enumerator identification (to reduce enum traffic).
  unsigned32 source;

  /// list of mods (possibly friendly names)
  char mod[MOD_LENGTH];

  /// equal mod are required for clients
  bool equalModRequired;

  /// selected game language id
  int32 language;
  /// selected mission difficulty
  int32 difficulty;
	//! Estimated time to mission end (in minutes)
	int32 timeleft;
	
  /// encode localized name
  void SetMissionName(const LocalizedString &str);
  
  /// decode localized name
  RString GetLocalizedMissionName() const;
} PACKED;

const size_t SESSION_PACKET_1 = offsetof(SessionPacket,numPlayers); // LEGACY_SESSION_PACKET
const size_t SESSION_PACKET_2 = offsetof(SessionPacket,mod);
const size_t SESSION_PACKET_3 = sizeof(SessionPacket);
const size_t SESSION_PACKET_SIZE = SESSION_PACKET_3;

/// Description of single session (will be stored within enumeration object).
struct NetSessionDescription : public SessionPacket
{

  /// Full address (string format).
  char address[32];

  /// IP adress in network format.
  unsigned32 ip;

  /// Last update time in milliseconds.
  unsigned32 lastTime;

  /// Actual ping (RTT) time.
  unsigned32 pingTime;

} PACKED;

/// Create player packet (will be sent from NetTranspClient to NetTranspServer).
struct CreatePlayerPacket : public MagicPacket
{

  /// Application-specific magic number.
  int32 magicApplication;

  char name[LEN_PLAYER_NAME];

  char password[LEN_PASSWORD_NAME];

  char mod[MOD_LENGTH];

  int32 actualVersion;

  int32 requiredVersion;

  int16 botClient;                          ///< 1 .. bot-client, 2 .. private-client (Xbox-secure only.

  int32 uniqueID;                           ///< Unique player ID (derived from system time and acknowledged by the server).

} PACKED;

/// Server response to CreatePlayer message (AckPlayer)
struct AckPlayerPacket : public MagicPacket
{

  /// Connection result (enum ConnectResult)
  int32 result;

  /// Player number (for future reconnection).
  int32 playerNo;

} PACKED;

/// Reconnect player packet (sent from NetTranspClient to NetTranspServer).
struct ReconnectPlayerPacket : public MagicPacket
{

  /// Player number (from first player acknowledgement).
  int32 playerNo;

} PACKED;

#pragma pack (pop,netPackets)

//---------------------------------------------------------------------------
//  NetSessionDescriptions:

/// Container for list of sessions.
class NetSessionDescriptions
{

protected:

  /// Current number of contained sessions.
  int _size;

  /// Sessions.
  NetSessionDescription _data[MAX_SESSIONS];

public:

  /// Constructor
  NetSessionDescriptions () { _size = 0; }

  /// Destructor
  ~NetSessionDescriptions () { Clear(); }

  /// Return current number of contained sessions
  int Size () { return _size; }

  /**
    Add new session.
    @return index of added session or -1 if session doesn't fit into container.
  */
  int Add ();

  /// Delete one session.
  void Delete ( int i );

  /// Delete all sessions.
  void Clear ();

  /// R/W access to single session.
  NetSessionDescription &operator [] ( int i )
  { return _data[i]; }

  /// R/O access to single session.
  const NetSessionDescription &operator [] ( int i ) const
  { return _data[i]; }

};

//---------------------------------------------------------------------------
//  NetSessionEnum:

/// Net implementation of NetTranspSessionEnum class
class NetSessionEnum : public NetTranspSessionEnum
{

protected:

  /// List of enumerated sessions.
  NetSessionDescriptions _sessions;

  /// Critical section for _sessions.
  PoCriticalSection _cs;

  /// Enter the critical section.
  inline void enter ()
  { _cs.enter(); }

  /// Leave the critical section.
  inline void leave ()
  { _cs.leave(); }

  /// Enumeration is running.
  bool _running;

  /// Magic number for application separation.
  int32 magicApp;

  /// source Xbox identification.
  unsigned32 source;

  /**
    Sends a request through the given channel.
    @param  br Existing (and well-configured) broadcast channel.
  */
  void sendRequest ( NetChannel *br );

  friend NetStatus bcastReceive ( NetMessage *msg, NetStatus event, void *data );

public:

  /// Constructor
  NetSessionEnum ();

  /// Destructor
  virtual ~NetSessionEnum ();

  /// Initializes enumeration data.
  virtual bool Init ( int magic =0 );

  /// Releases enumeration data.
  virtual void Done ();

  virtual RString IPToGUID ( RString ip, int port );

  /// Session enumeration is running?
  virtual bool RunningEnumHosts ()
  { return _running; }

  /**
    Starts enumeration of remote sessions.
    @param  ip Distant IP address to try (or NULL for broadcast only).
    @param  port Distant port to try.
    @param  hosts Array of distant hosts to try (or NULL if none).
  */
  virtual bool StartEnumHosts ( RString ip, int port, AutoArray<RemoteHostAddress> *hosts );

  /// Cancel session enumeration.
  virtual void StopEnumHosts ();

  /// Number of actually enumerated sessions.
  virtual int NSessions ();

  /// Retrieve all enumerated sessions.
  virtual void GetSessions ( AutoArray<SessionInfo> &sessions );

};

//---------------------------------------------------------------------------
//  NetClient:

/// Interface for network transport client class.
class NetClient : public NetTranspClient
{

protected:

  /// Last reported message time in seconds.
  unsigned64 lastMsgReported;

  /// Associated communication channel (peer-to-peer point).
  Ref<NetChannel> channel;

  /// AckPlayer result (CRNone if not finished yet).
  int ackPlayer;

  /// PlayerNo (playerID on the server - for reconnection purposes).
  int playerNo;

  /// Am I bot-client?
  bool amIBot;

  /// The session was terminated (by the server or channel drop)..
  bool sessionTerminated;

  /// The session was terminated (by the server or channel drop)..
  NetTerminationReason whySessionTerminated;

  /// The session was terminated by some specific reason
  char whySessionTerminatedStr[512];

  /// Linked list of received messages (can be NULL). Sorted by serial number.
  Ref<NetMessage> received;

  /// Inserts a new received message into 'received' sorted list..
  void insertReceived ( NetMessage *msg );

  /// Linked list of received message-parts to be merged.
  Ref<NetMessage> split;

  /// -> last received split message-part.
  NetMessage *lastSplit;

  /// Linked list of received message-parts to be merged.
  Ref<NetMessage> splitUrgent;

  /// -> last received split message-part.
  NetMessage *lastSplitUrgent;

  /// Critical section for received.
  PoCriticalSection rcvCs;

  /// Enter the critical section for received messages.
  inline void enterRcv ()
  { rcvCs.enter(); }

  /// Leave the critical section for received messages.
  inline void leaveRcv ()
  { rcvCs.leave(); }

  /// Linked list of sent messages (can be NULL).
  Ref<NetMessage> sent;

  /// Critical section for sent.
  mutable PoCriticalSection sndCs;

  /// Enter the critical section for sent messages.
  inline void enterSnd () const
  { sndCs.enter(); }

  /// Leave the critical section for sent messages.
  inline void leaveSnd () const
  { sndCs.leave(); }

  /// Magic number for application separation.
  int32 magicApp;

  friend NetStatus clientReceive ( NetMessage *msg, NetStatus event, void *data );
  friend NetStatus clientSendComplete ( NetMessage *msg, NetStatus event, void *data );

#ifdef NET_LOG_INFO
  int oldLatency;
  int oldThroughput;
  bool printAge;
#endif

#ifdef NET_LOG_TRANSP_STAT
  unsigned64 nextStatLog;
#endif

public:

  /// Constructor. Does virtually no initialization.
  NetClient ();

  /// Destructor.
  virtual ~NetClient ();

  /**
    Initialize the Client object.
    @param  address IP address:port of the distant server ("server.domain.tld:port").
    @param  password Password used to join the session.
    @param  botClient Is this "bot-client" (default client)?
    @param  privateClient Client connecting to "private slot" on the server.
    @param  port Remote port number! The correct used port number will be returned.
    @param  player Player name.
    @param  versionInfo Version info of the remote session.
    @param  magic Magic number to separate different applications.
  */
  virtual ConnectResult Init ( RString address, RString password, bool botClient, bool privateClient, int &port,
                               RString player, MPVersionInfo &versionInfo, int magic =0 );

  /// Create and initialize VoN voice client.
  virtual bool InitVoice ( VoiceMask *voiceMask, RawMessageCallback callback, bool isBotClient);
  /// Return true, if given remote player is speaking and voice is audible on local computer
  virtual bool IsVoicePlaying ( int player );
  /// Return true, if player on local computer is speaking and voice is recording for Voice Over Net
  virtual bool IsVoiceRecording ();
  virtual void IsVoice2D3DPlaying(int dpnid, bool &audible2D, bool &audible3D) const;
#if _ENABLE_CHEATS
  /// VoNSay script command to simulate more speakers without many players necessity
  virtual void VoNSay(int dpnid, int channel, int frequency, int seconds);
#endif
  virtual int CheckVoiceChannel(int dpnid, bool &audible2D, bool &audible3D) const;
  /// Switches on/off the recorder (microphone).
  virtual void VoiceCapture ( bool on );
  /// Switches on/off the voice transmition
  virtual void SetVoiceTransmition(bool val);
  /// Sets if voice is toggled on (user is NOT using push to talk button)
  virtual void SetVoiceToggleOn(bool val);
  /// sets threshold for silence analyzer (0.0f - 1.0f)
  virtual void SetVoNRecThreshold(float val);
  /// freqv CCItemTransmitter operates on
  virtual void SetVoNTransmitterFreq(float f);
  /// freqv CCItemTransmitter operates on
  virtual float GetVoNTransmitterFreq() float;
  /// maximal transmittion distance in meters
  virtual void SetVoNTransmitterRange(float range);
  virtual float GetVoNTransmitterRange2() float;
  /// Changes actual voice-mask for the encorder.
  virtual void ChangeVoiceMask ( VoiceMask *voiceMask );
  /// Sets position of remote player (affects sources)
  virtual void SetVoNPosition(int dpnid, Vector3Par pos, Vector3Par speed);
  /// Sets position of player (affects listener)
  virtual void SetVoNPlayerPosition(Vector3Par pos, Vector3Par speed);
  /// Sets obstruction of player (affects source)
  virtual void SetVoNObstruction(int dpnid, float obstruction, float occlusion, float deltaT);
  /// Updates volume and ear accomodation of the player (affects source)
  virtual void UpdateVoNVolumeAndAccomodation(int dpnid, float volume, float accomodation);
  /// Advance all VoN voices (even muted ones)
  virtual void AdvanceAllVoNSounds(float deltaT, bool paused, bool quiet);

  /// Sends the given user (raw) message to server.
  virtual Ref<NetMessage> SendMsg ( BYTE *buffer, int bufferSize, DWORD &msgID, NetMsgFlags flags );

  /// Gets actual send-queue statistics.
  virtual void GetSendQueueInfo ( int &nMsg, int &nBytes, int &nMsgG, int &nBytesG );

  /// Gets actual I/O channel statistics.
  virtual bool GetConnectionInfo ( int &latencyMS, int &throughputBPS );

	virtual void GetConnectionLimits(int &maxBandwidthPerClient);
  virtual void SetConnectionLimits(int maxBandwidthPerClient);
  /// Sets internal params for underlying NetChannel object.
  virtual void SetNetworkParams ( ParamEntryPar cfg );

  /// get local (my) address
  virtual bool GetLocalAddress( in_addr &addr ) const;

  /// get distant address (address of the server)
  virtual bool GetDistantAddress (in_addr &addr, int &port) const;

  /// get distant address along with port (of the server)
  virtual bool GetServerAddress( sockaddr_in &address ) const;

  /// get port where peer to peer channel is bind to
  virtual int GetVoicePort() const;

  /// get socket used for peer to peer communication
  virtual SOCKET GetVoiceSocket() const;

  /// set quality of codec in recorder (Speex)
  virtual void SetVoNCodecQuality(int quality);

  /// Last received message's age in seconds.
  virtual float GetLastMsgAge ();

  /// Last received message's age in seconds (used to eliminate "disconnect cheat")
  virtual float GetLastMsgAgeReliable () { return GetLastMsgAge(); }

  /// Last reported message's age in seconds.
  virtual float GetLastMsgAgeReported ();

  /// Remember time of reported message.
  virtual void LastMsgAgeReported ();

  virtual bool IsSessionTerminated ();

  virtual NetTerminationReason GetWhySessionTerminated ();
  virtual RString GetWhySessionTerminatedStr ();

  /// Processes received user messages.
  virtual void ProcessUserMessages ( UserMessageClientCallback *callback, void *context );

  /// Removes all received user messages.
  virtual void RemoveUserMessages ();

  /// Processes sent (and acknowledged) user messages.
  virtual void ProcessSendComplete ( SendCompleteCallback *callback, void *context );

  /// Removes all sent messages.
  virtual void RemoveSendComplete ();

  virtual RString GetStatistics ();

  virtual unsigned FreeMemory ();

  // called when application ends
  virtual void sendDisconnectMsg();
};

/*!
\patch 1.81 Date 8/7/2002 by Ondra
- Fixed: MP: Ghost might be left after user disconnected.
*/

#if 1
/// Empty key value (internal).
int ExplicitMapTraits<int,RefD<NetChannel> >::keyNull = -1;

/// For removed values (internal).
int ExplicitMapTraits<int,RefD<NetChannel> >::zombie  = -2;

#endif

//---------------------------------------------------------------------------
//  NetServer:

class ChannelSupport
{
public:

  /// Player (channel) id.
  int m_id;

  /// Linked list of received message-parts to be merged.
  Ref<NetMessage> m_split;

  /// -> last received split message-part.
  NetMessage *m_lastSplit;

  /// Linked list of received urgent message-parts to be merged.
  Ref<NetMessage> m_splitUrgent;

  /// -> last received urgent split message-part.
  NetMessage *m_lastSplitUrgent;

  ChannelSupport ()
  {
    m_id = -1;
    m_lastSplit = m_lastSplitUrgent = NULL;
  }

  ChannelSupport ( const ChannelSupport &from )
  {
    m_id              = from.m_id;
    m_split           = from.m_split;
    m_lastSplit       = from.m_lastSplit;
    m_splitUrgent     = from.m_splitUrgent;
    m_lastSplitUrgent = from.m_lastSplitUrgent;
  }

  bool operator== ( const ChannelSupport &sec ) const
  { return( m_id == sec.m_id ); }

};

TypeIsMovable(ChannelSupport);

unsigned64 ExplicitMapTraits<unsigned64,ChannelSupport>::keyNull = (unsigned64)-1;
unsigned64 ExplicitMapTraits<unsigned64,ChannelSupport>::zombie  = (unsigned64)-2;
ChannelSupport ExplicitMapTraits<unsigned64,ChannelSupport>::null;

void SessionPacket::SetMissionName(const LocalizedString &str)
{
  missionLocalizedType = ' '+str._type;
  strcpy(missionId,str._id);

}

RString SessionPacket::GetLocalizedMissionName() const
{
  int type = missionLocalizedType-' ';
  saturate(type,LocalizedString::PlainText,LocalizedString::Stringtable);
  LocalizedString str(LocalizedString::Type(type),missionId);
  return str.GetLocalizedValue();
}

/// Socket implementation for network transport server class.
class NetServer : public NetTranspServer
{

protected:

  /// Session password
  RString _password;

  /// Session info in network format.
  SessionPacket session;

  /// Critical section for user map, session info, added & deleted users.
  PoCriticalSection usrCs;

  /// Enter the critical section for user map.
  inline void enterUsr ()
  { usrCs.enter(); }

  /// Leave the critical section for user map.
  inline void leaveUsr ()
  { usrCs.leave(); }

  /// Port on which the session is running.
  unsigned16 sessionPort;

  /// Name of session (contains name of player and IP address of host).
  RString sessionName;

  /// Linked list of received messages (can be NULL). Sorted by serial number. The list is shared by all players.
  Ref<NetMessage> received;

  /// Inserts a new received message into 'received' sorted list..
  void insertReceived ( NetMessage *msg );

  /// NetChannel support (splitted messages, etc.).
  ExplicitMap<unsigned64,ChannelSupport,true,MemAllocSafe> m_support;

  /// Critical section for received and m_support.
  PoCriticalSection rcvCs;

  /// Enter the critical section for received messages.
  inline void enterRcv ()
  { rcvCs.enter(); }

  /// Leave the critical section for received messages.
  inline void leaveRcv ()
  { rcvCs.leave(); }

  /// Linked list of sent messages (can be NULL).
  Ref<NetMessage> sent;

  /// Critical section for sent.
  PoCriticalSection sndCs;

  /// Enter the critical section for sent messages.
  inline void enterSnd ()
  { sndCs.enter(); }

  /// Leave the critical section for sent messages.
  inline void leaveSnd ()
  { sndCs.leave(); }

  /// Mapping from user ID# to NetChannels.
  ExplicitMap<int,RefD<NetChannel>,true,MemAllocSafe> users;

  /// Set of private users.
  ImplicitSet<int,intToUnsigned,true,MemAllocSafe> privateUsers;
  //ImplicitMap<int,int,identityInt,true,MemAllocSafe> privateUsers;

  /// ID of 'bot' client or 0.
  int botId;

  /// Info about new players.
  AutoArray<CreatePlayerInfo,MemAllocSafe> _createPlayers;

  /// Info about removed players.
  AutoArray<DeletePlayerInfo,MemAllocSafe> _deletePlayers;

  /// Magic number for application separation.
  int32 magicApp;

  /// Respond to enumeration request?
  bool m_enumResponse;

  /**
    Maps NetChannel to player ID#.
    @return Player # or <code>-1</code> if not found.
  */
  int channelToPlayer ( NetChannel *ch );

  friend NetStatus bcastReceive ( NetMessage *msg, NetStatus event, void *data );
  friend NetStatus ctrlReceive ( NetMessage *msg, NetStatus event, void *data );
  friend NetStatus serverReceive ( NetMessage *msg, NetStatus event, void *data );
  friend NetStatus destroyPlayerCallback ( NetMessage *msg, NetStatus event, void *data );
  friend NetStatus serverSendComplete ( NetMessage *msg, NetStatus event, void *data );

  /// Complete player-destroy sequence (with notification message to the player).
  void destroyPlayer ( NetChannel *ch, NetTerminationReason reason, const char *reasonStr=NULL );

  /// Final player-destroying (w/o notification message to the player).
  void finishDestroyPlayer ( int player );

  /// diagnostics - log current state of "users" variable
  void logUsers();

#ifdef NET_LOG_INFO
  int oldLatency;
  int oldThroughput;
#endif

#ifdef DEDICATED_STAT_LOG
  unsigned64 nextConsoleLog;
#endif

#ifdef NET_LOG_TRANSP_STAT
  unsigned64 nextStatLog;
  bool forceLog;
  bool inGetConnection;
#endif

public:

  /// Constructor. Does virtually no initialization.
  NetServer ();

  /// Destructor.
  virtual ~NetServer ();

  /// Initialize the server object.
  virtual bool Init ( int port, RString password,
                      const XNADDR &addr, const XNKID &kid, const XNKEY &key,
                      int maxPlayers, int maxPrivate, RString playerName, int language, int difficulty,
                      MPVersionInfo &versionInfo, bool equalModRequired, bool enumResponse, int magic =0 );
	void UpdateMaxPlayers(int maxPlayers, int maxPrivate);
	int GetMaxPrivateSlots();
  void UpdateLockedOrPassworded(bool lock, bool passworded);

  /// Create and initialize VoN voice server.
  virtual bool InitVoice ();
  virtual void ProcessVoicePlayers ( CreateVoicePlayerCallback *callback, void *context );
  virtual void RemoveVoicePlayers ();
  virtual void GetTransmitTargets ( int from, AutoArray<int,MemAllocSA> &to );
  virtual void SetTransmitTargets ( int from, AutoArray<int,MemAllocSA> &to );
  virtual void UpdateTransmitTargets();
  virtual void GetVoNTopologyDiag(QOStrStream &out, Array<ConvPair> &convTable);

  virtual void disconnectAllPlayers();

  virtual int GetSessionPort ()
  { return sessionPort; }

  virtual RString GetSessionName ()
  { return sessionName; }

  virtual Ref<NetMessage> SendMsg ( int to, BYTE *buffer, int bufferSize, DWORD &msgID, NetMsgFlags flags );

  virtual void CancelAllMessages ();

  virtual void GetSendQueueInfo ( int to, int &nMsg, int &nBytes, int &nMsgG, int &nBytesG );

  virtual bool GetConnectionInfo ( int to, int &latencyMS, int &throughputBPS );

  virtual void GetConnectionLimits(int &maxBandwidthPerClient);
  virtual void SetConnectionLimits(int maxBandwidthPerClient);
  
  /// Last received message's age in seconds (used to eliminate "disconnect cheat")
  virtual float GetLastMsgAgeReliable ( int player );

  /// Sets internal params for underlying NetChannel object.
  virtual void SetNetworkParams ( ParamEntryPar cfg );

  virtual void UpdateSessionDescription (
    int state, RString gameType, const LocalizedString &mission, RString island,
    int timeleft, int difficulty
  );

  virtual bool GetURL ( char *address, DWORD addressLen );

  virtual bool GetServerAddress(sockaddr_in &address);

  virtual bool GetClientAddress(int client, sockaddr_in &address);

  virtual void KickOff ( int player, NetTerminationReason reason, const char *reasonStr=NULL );

  virtual void ProcessUserMessages ( UserMessageServerCallback *callback, void *context );

  virtual void RemoveUserMessages ();

  virtual void ProcessSendComplete ( SendCompleteCallback *callback, void *context );

  virtual void RemoveSendComplete ();

  virtual void ProcessPlayers ( CreatePlayerCallback *callbackCreate, DeletePlayerCallback *callbackDelete, void *context );

  virtual void RemovePlayers ();

  virtual RString GetStatistics ( int player );
  
  virtual void NotifyChannelDataSent(int player, size_t size);

  virtual unsigned FreeMemory ();

  virtual RefD<NetChannel> playerToChannel ( int player );

};

void NetServer::NotifyChannelDataSent(int player, size_t size)
{
  RefD<NetChannel> channel;
  users.get(player,channel);
  if (channel)
  {
    channel->dataSentAck(size);
  }
}

//---------------------------------------------------------------------------
//  NetPeerToPeer:

#ifdef PEER2PEER

/**
  * Server object: listens on the given port and processes p2p requests.
  * Only one instance of server is avaliable (singleton).
  */
class NetPeerToPeerServer : public PeerToPeerServer
{
protected:

  /**
    * Set of active p2p channels.
    * (they should automatically fade away after some time..)
    */
  ExplicitMap<int,RefD<NetChannel>,true,MemAllocSafe> m_channels;

  /// Next used channel-id.
  int m_nextChannel;

  /// Critical section for m_channels etc.
  PoCriticalSection m_chCs;

  /// Enter the critical section for channel manipulation.
  inline void enterCh ()
  { m_chCs.enter(); }

  /// Leave the critical section for channel manipulation.
  inline void leaveCh ()
  { m_chCs.leave(); }

  /// Received messages waiting for processing (linked list).
  Ref<NetMessage> m_received;

  /// Critical section for m_received.
  PoCriticalSection m_rcvCs;

  /// Enter the critical section for received messages.
  inline void enterRcv ()
  { m_rcvCs.enter(); }

  /// Leave the critical section for received messages.
  inline void leaveRcv ()
  { m_rcvCs.leave(); }

  /// Channel support (split messages etc.).
  ExplicitMap<unsigned64,ChannelSupport,true,MemAllocSafe> m_support;

  friend NetStatus p2pReceive ( NetMessage *msg, NetStatus event, void *data );

public:

  /// Initializes network layer, starts the listening thread, etc.
  NetPeerToPeerServer ( const XNKID &kid );

  /**
    * Cancels all pending requests, stops the listening thread,
    * frees all network-related resources, etc.
    */
  virtual ~NetPeerToPeerServer ();

  /**
    * Sends the given answer to the recipient (which had asked me before).
    * Returns true if succeeded (there's no transport-garancy at this moment).
    */
  virtual bool SendAnswer ( int channel, BYTE *data, int dataSize );

  /// Processes all buffered incoming messages.
  virtual void ProcessIncomingMessages ( PeerToPeerIncomingCallback *callback, void *context );

};

/**
  * Client object - used to communicate with one individual server.
  * Only one client object can be available at the same time (singleton).
  */
class NetPeerToPeerClient : public PeerToPeerClient
{
protected:

  /// Working net-channel.
  RefD<NetChannel> m_channel;

  /// kid was successfully registered?
  bool m_registered;

  /// Received messages waiting for processing (linked list).
  Ref<NetMessage> m_received;

  /// Critical section for m_received.
  PoCriticalSection m_rcvCs;

  /// Enter the critical section for received messages.
  inline void enterRcv ()
  { m_rcvCs.enter(); }

  /// Leave the critical section for received messages.
  inline void leaveRcv ()
  { m_rcvCs.leave(); }

  /// Linked list of received message-parts to be merged.
  Ref<NetMessage> m_split;

  /// -> last received split message-part.
  NetMessage *m_lastSplit;

  friend NetStatus p2pReceive ( NetMessage *msg, NetStatus event, void *data );

public:

  /// Initializes network layer but doesn't start the request yet..
  NetPeerToPeerClient ( const XNKID &kid, const XNKEY &key, const XNADDR &addr );

  /**
    * Cancels the client,
    * frees all network-related resources, etc.
    */
  virtual ~NetPeerToPeerClient ();

  /**
    * Sends the given request to the server.
    * Returns true if no formal error occurs (but there's no transport-garancy at this moment).
    */
  virtual bool SendRequest ( BYTE *data, int dataSize );

  /**
    * Processes all buffered incoming messages.
    * Should be called periodically (some cleanup will be performed inside..)
    */
  virtual void ProcessIncomingMessages ( PeerToPeerIncomingCallback *callback, void *context );

};

#endif    // PEER2PEER

//---------------------------------------------------------------------------
//  Global pools and peers etc.:

/// The entire broadcast communication (enumeration requests/responses, ..).
NetStatus bcastReceive ( NetMessage *msg, NetStatus event, void *data );

NetStatus clientReceive ( NetMessage *msg, NetStatus event, void *data );
NetStatus serverReceive ( NetMessage *msg, NetStatus event, void *data );
NetStatus ctrlReceive ( NetMessage *msg, NetStatus event, void *data );

#ifdef PEER2PEER
NetStatus p2pReceive ( NetMessage *msg, NetStatus event, void *data );
#endif

static PoCriticalSection LockDecl(poolLock,"::poolLock");
                                            ///< locking of 'pool', 'bcastPeer', 'gamePeer'

static RefD<NetPool> pool;                  ///< Global network pool.

static RefD<NetPeer> bcastPeer;             ///< NetPeer for enumeration (with broadcast capability).

static RefD<NetPeer> gamePeer;              ///< NetPeer for server/client (regular game communication).

static RefD<NetPeer> p2pPeer;               ///< NetPeer for peer2peer communication.

#if VOICE_OVER_NET

static RefD<VoNSystemXbox> von;             ///< Global VoN factory.

DWORD commInPort = (DWORD)-1;               ///< Actual microphone's port or -1 if communicator is not present.
DWORD commOutPort = (DWORD)-1;              ///< Actual replayer's port or -1 if voice should be sent to TV

#endif // VOICE_OVER_NET

#if _ENABLE_REPORT
int VoiceServerGetReceivedDataCount()
{
#if VOICE_OVER_NET
  if (!von) return 0;
  VoNServer *server = von->getServer();
  if (!server) return 0;
  return server->GetReceivedDataCount();
#else
  return 0;
#endif
}
int VoiceServerGetReceivedDataSize()
{
#if VOICE_OVER_NET
  if (!von) return 0;
  VoNServer *server = von->getServer();
  if (!server) return 0;
  return server->GetReceivedDataSize();
#else
  return 0;
#endif
}
int VoiceServerGetReceivedCtrlCount()
{
#if VOICE_OVER_NET
  if (!von) return 0;
  VoNServer *server = von->getServer();
  if (!server) return 0;
  return server->GetReceivedCtrlCount();
#else
  return 0;
#endif
}
int VoiceServerGetReceivedCtrlSize()
{
#if VOICE_OVER_NET
  if (!von) return 0;
  VoNServer *server = von->getServer();
  if (!server) return 0;
  return server->GetReceivedCtrlSize();
#else
  return 0;
#endif
}
int VoiceServerGetSentDataCount()
{
#if VOICE_OVER_NET
  if (!von) return 0;
  VoNServer *server = von->getServer();
  if (!server) return 0;
  return server->GetSentDataCount();
#else
  return 0;
#endif
}
int VoiceServerGetSentDataSize()
{
#if VOICE_OVER_NET
  if (!von) return 0;
  VoNServer *server = von->getServer();
  if (!server) return 0;
  return server->GetSentDataSize();
#else
  return 0;
#endif
}
int VoiceServerGetSentCtrlCount()
{
#if VOICE_OVER_NET
  if (!von) return 0;
  VoNServer *server = von->getServer();
  if (!server) return 0;
  return server->GetSentCtrlCount();
#else
  return 0;
#endif
}
int VoiceServerGetSentCtrlSize()
{
#if VOICE_OVER_NET
  if (!von) return 0;
  VoNServer *server = von->getServer();
  if (!server) return 0;
  return server->GetSentCtrlSize();
#else
  return 0;
#endif
}
void VoiceServerResetStats()
{
#if VOICE_OVER_NET
  if (!von) return;
  VoNServer *server = von->getServer();
  if (!server) return;
  server->ResetStats();
#endif
}
int VoiceClientGetReceivedDataCount()
{
#if VOICE_OVER_NET
  if (!von) return 0;
  VoNClient *client = von->getClient();
  if (!client) return 0;
  return client->GetReceivedDataCount();
#else
  return 0;
#endif
}
int VoiceClientGetReceivedDataSize()
{
#if VOICE_OVER_NET
  if (!von) return 0;
  VoNClient *client = von->getClient();
  if (!client) return 0;
  return client->GetReceivedDataSize();
#else
  return 0;
#endif
}
int VoiceClientGetReceivedCtrlCount()
{
#if VOICE_OVER_NET
  if (!von) return 0;
  VoNClient *client = von->getClient();
  if (!client) return 0;
  return client->GetReceivedCtrlCount();
#else
  return 0;
#endif
}
int VoiceClientGetReceivedCtrlSize()
{
#if VOICE_OVER_NET
  if (!von) return 0;
  VoNClient *client = von->getClient();
  if (!client) return 0;
  return client->GetReceivedCtrlSize();
#else
  return 0;
#endif
}
int VoiceClientGetSentDataCount()
{
#if VOICE_OVER_NET
  if (!von) return 0;
  VoNClient *client = von->getClient();
  if (!client) return 0;
  return client->GetSentDataCount();
#else
  return 0;
#endif
}
int VoiceClientGetSentDataSize()
{
#if VOICE_OVER_NET
  if (!von) return 0;
  VoNClient *client = von->getClient();
  if (!client) return 0;
  return client->GetSentDataSize();
#else
  return 0;
#endif
}
int VoiceClientGetSentCtrlCount()
{
#if VOICE_OVER_NET
  if (!von) return 0;
  VoNClient *client = von->getClient();
  if (!client) return 0;
  return client->GetSentCtrlCount();
#else
  return 0;
#endif
}
int VoiceClientGetSentCtrlSize()
{
#if VOICE_OVER_NET
  if (!von) return 0;
  VoNClient *client = von->getClient();
  if (!client) return 0;
  return client->GetSentCtrlSize();
#else
  return 0;
#endif
}
void VoiceClientResetStats()
{
#if VOICE_OVER_NET
  if (!von) return;
  VoNClient *client = von->getClient();
  if (!client) return;
  client->ResetStats();
#endif
}
#endif // _ENABLE_REPORT

void SafeHeapBeginUse();
void SafeHeapEndUse();

void initNetwork ()
{
  initNetworkXbox();
}

void doneNetwork ()
{
  doneNetworkXbox();
}

void createPool ()
{
  if ( !pool )
  {
    SafeHeapBeginUse();
    pool = new NetPool( new PeerChannelFactoryXbox, NULL, NULL );
  }
}

void destroyVoice ()
{
#if VOICE_OVER_NET
  if ( von )
  {
#ifdef NET_LOG_VOICE
    NetLog("destroyPool: destroying VoNSystem instance");
#endif
    von = NULL;
  }
#endif
}

void destroyPool ()
{
  poolLock.enter();
  destroyVoice();
  if ( gamePeer )
  {
    if ( pool )
      pool->deletePeer(gamePeer);
    else
      gamePeer->close();
    gamePeer = NULL;
  }
  if ( bcastPeer )
  {
    if ( pool )
      pool->deletePeer(bcastPeer);
    else
      bcastPeer->close();
    bcastPeer = NULL;
  }
  if ( p2pPeer )
  {
    if ( pool )
      pool->deletePeer(p2pPeer);
    else
      p2pPeer->close();
    p2pPeer = NULL;
  }
  pool = NULL;
  poolLock.leave();
#ifdef NET_LOG
  netLogFlush();
#endif
  SafeHeapEndUse();
}

static void setupPortBitMask ( BitMask &mask, bool game, bool p2p =false )
{
  mask.empty();
  if ( game )
    mask.on(XBOX_GAME_PORT);
  else
#ifdef PEER2PEER
    mask.range( p2p ? XBOX_P2P_PORT : XBOX_BCAST_PORT,NN_NUM_PORTS_TO_TRY,true);
#else
    mask.range(XBOX_BCAST_PORT,NN_NUM_PORTS_TO_TRY,true);
#endif
}

bool CheckRawMessage(char *data, int len, struct sockaddr_in *from)
{
  return false;
}

static NetPeer *getBcastPeer ()
  // for enumeration only, should be called inside of poolLock.enter()
{
  if ( !bcastPeer )
  {
    BitMask portMask;
    setupPortBitMask(portMask,false);
    bcastPeer = getPool()->createPeer(&portMask, false, &CheckRawMessage); // UDP
    if ( bcastPeer )
    {
      NetChannel *ctrl = bcastPeer->getBroadcastChannel();
      if ( ctrl ) ctrl->setProcessRoutine(bcastReceive);
    }
  }
  return bcastPeer;
}

static NetPeer *getGamePeer ()
  // for client & servers, should be called inside of poolLock.enter()
{
  if ( !gamePeer )
  {
    BitMask portMask;
    setupPortBitMask(portMask,true);
    gamePeer = getPool()->createPeer(&portMask, true, &CheckRawMessage);   // VDP
    if ( gamePeer )
    {
      NetChannel *ctrl = gamePeer->getBroadcastChannel();
      if ( ctrl ) ctrl->setProcessRoutine(ctrlReceive);
    }
  }
  return gamePeer;
}

#ifdef PEER2PEER

static NetPeer *getP2pPeer ()
  // for p2p sharing, should be called inside of poolLock.enter()
{
  if ( !p2pPeer )
  {
    BitMask portMask;
    setupPortBitMask(portMask,false,true);
    p2pPeer = getPool()->createPeer(&portMask, true, &CheckRawMessage);    // VDP
    if ( p2pPeer )
    {
      NetChannel *ctrl = p2pPeer->getBroadcastChannel();
      if ( ctrl ) ctrl->setProcessRoutine(p2pReceive);
    }
  }
  return p2pPeer;
}

#endif    // PEER2PEER

//---------------------------------------------------------------------------
//  Global enum, server & client instances (NetTransp* objects):

/// Actual enumeration instance (can be NULL).
static NetSessionEnum *_enum = NULL;

/// Actual client instance (can be NULL).
static NetClient *_client = NULL;

/// NetServer instance to process control messages.
static NetServer *_server = NULL;

#ifdef PEER2PEER

/// NetPeerToPeerServer instance (could be NULL most times - if nothing is offered).
static NetPeerToPeerServer *_p2pServer = NULL;

/// NetPeerToPeerClient instance (used only while processing p2p-requests to one server).
static NetPeerToPeerClient *_p2pClient = NULL;

/// Sets actual peer-to-peer server instance (can be NULL).
void setPeerToPeerServer ( NetPeerToPeerServer *p2pServer )
{
  poolLock.enter();
  _p2pServer = p2pServer;
  if ( _p2pServer )
  {
    getPool();
    GET_P2P_PEER();
  }
  poolLock.leave();
}

/// Sets actual peer-to-peer client instance (can be NULL).
void setPeerToPeerClient ( NetPeerToPeerClient *p2pClient )
{
  poolLock.enter();
  _p2pClient = p2pClient;
  if ( _p2pClient )
  {
    getPool();
    GET_P2P_PEER();
  }
  poolLock.leave();
}

#endif    // PEER2PEER

//---------------------------------------------------------------------------
//  Receiving routines:

/*!
\patch_internal 1.51 Date 4/17/2002 by Pepca
- Fixed: Bugs in player registration at the server (multiple registration can occur). [Sockets]
- Fixed: Disconnected user is now accepted by the server immediately when [s]he makes another try.
[Sockets]
\patch_internal 1.51 Date 4/18/2002 by Pepca
- Added: Each player has its "unique identifier" based on current system time (and verified in
initial player<->server negotiation). [Sockets]
\patch_internal 1.51 Date 4/18/2002 by Pepca
- Fixed: Player-count limit is now interpreted correctly by the server (it was ignored in previous
versions). [Sockets]
*/

/// Control (broadcast) data received by the enumerator and server...
NetStatus bcastReceive ( NetMessage *msg, NetStatus event, void *data )
{
        // thread-critical: NetMessagePool::pool()->newMessage(), NetMessage::send(), strncmp(),
        //                  GET_ENUM_PEER()->getPool()->createChannel(), AutoArray::Add()
    if ( (!_enum || !_enum->_running) && !_server ||
         !msg || msg->getLength() < 4 ||
         !(msg->getFlags() & MSG_MAGIC_FLAG) )
        return nsNoMoreCallbacks;           // fatal message error

    unsigned32 magic = *(unsigned32*)msg->getData();
    struct sockaddr_in distant;
    msg->getDistant(distant);
    unsigned32 ip = ntohl(distant.sin_addr.s_addr);
    //unsigned16 port = ntohs(distant.sin_port);

#ifdef NET_LOG_CTRL_RECEIVE
#  ifdef NET_LOG_BRIEF
    NetLog("Pe(%u)::rCtrlB(%u.%u.%u.%u:%u,%u,%u,%x,%x)",
           GET_ENUM_PEER()->getPeerId(),
           (unsigned)IP4(distant),(unsigned)IP3(distant),(unsigned)IP2(distant),(unsigned)IP1(distant),(unsigned)PORT(distant),
           msg->getLength(),msg->getSerial(),(unsigned)msg->getFlags(),magic);
#  else
    NetLog("Peer(%u)::bcastReceive: received control message (from=%u.%u.%u.%u:%u, len=%3u, serial=%4u, flags=%04x, magic=%x)",
           GET_ENUM_PEER()->getPeerId(),
           (unsigned)IP4(distant),(unsigned)IP3(distant),(unsigned)IP2(distant),(unsigned)IP1(distant),(unsigned)PORT(distant),
           msg->getLength(),msg->getSerial(),(unsigned)msg->getFlags(),magic);
#  endif
#endif

    switch ( magic ) {

        case MAGIC_ENUM_REQUEST:            // enumeration request => server
            if ( !_server || !_server->m_enumResponse ) break;
            if ( msg->getLength() == sizeof(EnumPacket) ) {
                EnumPacket *ep = (EnumPacket*)msg->getData();
                _server->enterUsr();
                if ( ep->magicApplication == _server->magicApp ) {
                    Ref<NetMessage> out = NetMessagePool::pool()->newMessage(SESSION_PACKET_SIZE,msg->getChannel());
                    if ( out ) {
                        distant.sin_addr.s_addr = INADDR_BROADCAST;
                        out->setDistant(distant);
                        out->setFlags(MSG_ALL_FLAGS,MSG_FROM_BCAST_FLAG|MSG_TO_BCAST_FLAG|MSG_MAGIC_FLAG);
                        _server->session.source     = ep->source;   // send back enum-identification
                        _server->session.magic      = MAGIC_ENUM_RESPONSE;
                        _server->session.request    = msg->getSerial();
                        _server->session.numPlayers = _server->users.card();
                        // bot client is always included to maxPlayers and excluded to numPlayers
#if _ENABLE_DEDICATED_SERVER
                        if (IsDedicatedServer())
                        {
                          // exclude the bot client
                          _server->session.maxPlayers--;                
                        }
                        else
#endif
                        {
                          // include the bot client
                          _server->session.numPlayers++;                
                        }
                        _server->session.numPrivate = _server->privateUsers.card();
                        out->setData((unsigned8*)&(_server->session),SESSION_PACKET_SIZE);
                        out->send(true);    // urgent message
#if _ENABLE_DEDICATED_SERVER
                        if (IsDedicatedServer())
                        {
                          // reset maxPlayers (is persistent)
                          _server->session.maxPlayers++;                
                        }
#endif
                        }
                    }
                _server->leaveUsr();
                }
            break;

        case MAGIC_ENUM_RESPONSE:           // enumeration response => enumerator
            if ( !_enum || !_enum->_running ) break;

            if ( msg->getLength() != SESSION_PACKET_2 &&
                 msg->getLength() != SESSION_PACKET_3 )
              break; // wrong size

              // default mod required:
            char mod[MOD_LENGTH]; mod[0] = (char)0;
            bool equalModRequired = false;
            if ( msg->getLength() >= SESSION_PACKET_3 )
            {
              strncpy(mod,((SessionPacket *)msg->getData())->mod,MOD_LENGTH);
              mod[MOD_LENGTH-1] = (char)0;
              equalModRequired = ((SessionPacket *)msg->getData())->equalModRequired;
            }

            SessionPacket *s = (SessionPacket *)msg->getData();
            if ( s->source == _enum->source ) {
                _enum->enter();

                    // search for existing session record
                int iFound = -1;
                for ( int i = 0; i < _enum->_sessions.Size(); i++ )
                    if ( !memcmp(&(_enum->_sessions[i].xaddr),&(s->xaddr),sizeof(XNADDR)) ) {
                        iFound = i;
                        break;
                        }

                unsigned64 reqTime = msg->getChannel()->getMessageTime(s->request);
                    // actual ping time in milliseconds:
                unsigned pingTime = reqTime ? (unsigned)( (msg->getTime() - reqTime) / 1000 ) : 0;

                if ( iFound < 0 ) {                     // not found
                    iFound = _enum->_sessions.Add();
                    if ( iFound < 0 ) {                 // too much sessions encountered
                        _enum->leave();
                        return nsNoMoreCallbacks;
                        }
                    NetSessionDescription &ndesc = _enum->_sessions[iFound];
                    ndesc.ip       = ip;
                    ndesc.port     = XBOX_GAME_PORT;
                    ndesc.pingTime = pingTime;
                    XNetInAddrToString(distant.sin_addr, ndesc.address, sizeof(ndesc.address));
                    }

                NetSessionDescription &desc = _enum->_sessions[iFound];

//                sprintf(desc.name,s->name,desc.address);
                strncpy(desc.name, s->name, LEN_SESSION_NAME);
                desc.name[LEN_SESSION_NAME - 1] = 0;

                desc.actualVersion   = s->actualVersion;
                desc.requiredVersion = s->requiredVersion;
                strncpy(desc.gameType,s->gameType,LEN_GAMETYPE_NAME);
                desc.gameType[LEN_GAMETYPE_NAME-1] = 0;
                strncpy(desc.missionId,s->missionId,sizeof(desc.missionId)-1);
                desc.missionId[sizeof(desc.missionId)-1] = 0;
                desc.missionLocalizedType = s->missionLocalizedType;
                strncpy(desc.island,s->island,LEN_ISLAND_NAME);
                desc.island[LEN_ISLAND_NAME-1] = 0;
                strncpy(desc.mod,mod,MOD_LENGTH);
                desc.mod[MOD_LENGTH-1] = 0;
                desc.equalModRequired = equalModRequired;
                desc.serverState     = s->serverState;
                desc.maxPlayers      = s->maxPlayers;
                desc.maxPrivate      = s->maxPrivate;
                desc.numPlayers      = s->numPlayers;
                desc.numPrivate      = s->numPrivate;
                desc.password        = s->password;
                desc.lastTime        = (unsigned32)(getSystemTime() / 1000);
                desc.pingTime        = (3*desc.pingTime + pingTime + 2) >> 2;
                desc.language = s->language;
                desc.difficulty = s->difficulty;
	              desc.timeleft = s->timeleft;
                desc.xaddr           = s->xaddr;
                desc.kid             = s->kid;
                desc.key             = s->key;
                _enum->leave();
                }
            break;

        }

    return nsNoMoreCallbacks;
}

/// Control data received by the server...
NetStatus ctrlReceive ( NetMessage *msg, NetStatus event, void *data )
{
        // thread-critical: NetMessagePool::pool()->newMessage(), NetMessage::send(), strncmp(),
        //                  GET_SERVER_PEER()->getPool()->createChannel(), AutoArray::Add()
    if ( !_server ||
         !msg || msg->getLength() < 4 ||
         !(msg->getFlags() & MSG_MAGIC_FLAG) )
        return nsNoMoreCallbacks;           // fatal message error

    unsigned32 magic = *(unsigned32*)msg->getData();
    struct sockaddr_in distant;
    msg->getDistant(distant);
    //unsigned32 ip = ntohl(distant.sin_addr.s_addr);
    //unsigned16 port = ntohs(distant.sin_port);

#ifdef NET_LOG_CTRL_RECEIVE
#  ifdef NET_LOG_BRIEF
    NetLog("Pe(%u):rCtrlS(%u.%u.%u.%u:%u,%u,%x)",
           GetServerPeerId(),
           (unsigned)IP4(distant),(unsigned)IP3(distant),(unsigned)IP2(distant),(unsigned)IP1(distant),(unsigned)PORT(distant),
           msg->getLength(),magic);
#  else
    NetLog("Peer(%u)::ctrlReceive: received control message (from=%u.%u.%u.%u:%u, len=%3u, serial=%4u, flags=%04x, magic=%x)",
           GetServerPeerId(),
           (unsigned)IP4(distant),(unsigned)IP3(distant),(unsigned)IP2(distant),(unsigned)IP1(distant),(unsigned)PORT(distant),
           msg->getLength(),msg->getSerial(),(unsigned)msg->getFlags(),magic);
#  endif
#endif

    switch ( magic ) {

        case MAGIC_CREATE_PLAYER:           // create player => server
            if ( msg->getLength() == sizeof(CreatePlayerPacket) ) {
                CreatePlayerPacket *cpp = (CreatePlayerPacket*)msg->getData();
                _server->enterUsr();        // lock the user, session etc. structures for a long time..
                // avoid corrupted or modified packages (usage of reserved ids could cause crash, freeze etc.)
                if ( cpp->magicApplication != _server->magicApp || cpp->uniqueID - UNIQUE_TO_TRY - RESERVED_IDS < 0) {
                    _server->leaveUsr();
                    break;
                    }
                ConnectResult result;
                    // check new player's attributes:
                if ( cpp->actualVersion < _server->session.requiredVersion ||
                  _server->session.actualVersion < cpp->requiredVersion ||
                  _server->session.equalModRequired && stricmp(_server->session.mod, cpp->mod) != 0)
                    result = CRVersion;
                else
                    if ( strncmp(cpp->password,_server->_password,LEN_PASSWORD_NAME) != 0 )
                        result = CRPassword;
                    else
                        result = CROK;
                    // create a new player:
                int player = 0;             // dummy player ID
                int playerEnd;
                NetChannel *ch = NULL;
                if ( result == CROK ) {
                    poolLock.enter();
                    NetPeer *peer = GET_SERVER_PEER();
                    Assert( peer );
                    RefD<NetChannel> findCh;
                    ch = peer->findChannel(distant);
#ifdef NET_LOG_SERVER
#  ifdef NET_LOG_BRIEF
                    NetLog("Pe(%u):try('%s',%d,%d)",
                      peer->getPeerId(),cpp->name,cpp->uniqueID,ch?1:0);
#  else
                    NetLog("Peer(%u): new player try, name = '%s', sentID = %d, exist = %d",
                      peer->getPeerId(),cpp->name,cpp->uniqueID,ch?1:0);
#  endif
#endif
                    if ( ch ) {             // this channel is being used!
#ifdef NET_LOG_SERVER
                      {
                        IteratorState it;
                        int pl;
                        _server->users.getFirst(it,findCh,&pl);
                        do
                        {
                          struct sockaddr_in dist;
                          findCh->getDistantAddress(dist);
                          NetLog("Pe(%u):%d->%u(%u.%u.%u.%u:%u)",
                                peer->getPeerId(),pl,findCh->getChannelId(),
                                (unsigned)IP4(dist),(unsigned)IP3(dist),(unsigned)IP2(dist),(unsigned)IP1(dist),(unsigned)PORT(dist)
                                );
                        } while ( _server->users.getNext(it,findCh,&pl) );
                      }
#endif
                        player = cpp->uniqueID;
                        playerEnd = player - UNIQUE_TO_TRY;
                        do
                            if ( _server->users.get(player,findCh) && (NetChannel*)findCh == ch ) break;
                        while ( --player != playerEnd );
                        if ( player != playerEnd ) { // found => refuse it!
                            poolLock.leave();
                            _server->leaveUsr();
#ifdef NET_LOG_SERVER
#  ifdef NET_LOG_BRIEF
                            NetLog("Pe(%u):ignore('%s',%d,%d)",
                                   peer->getPeerId(),cpp->name,cpp->uniqueID,ch?1:0);
#  endif
#endif
                            break;          // duplicate CreatePlayer message => refuse it
                            }
                            // new connection from the same IP:port => kick off the old player
#ifdef NET_LOG_SERVER
#  ifdef NET_LOG_BRIEF
                        NetLog("Pe(%u):kick(%d)",
                               peer->getPeerId(),_server->channelToPlayer(ch));
#  endif
#endif
                        _server->finishDestroyPlayer(_server->channelToPlayer(ch));
                        ch = NULL;
                        poolLock.leave();
                        _server->leaveUsr();
                        SLEEP_MS(DESTROY_WAIT); // waiting till my boss destroys the old player (to be sure)..
                        _server->enterUsr();
                        poolLock.enter();
                        }
                        // creating a new player: first determine its ID..
                    player = cpp->uniqueID;
                    playerEnd = player + UNIQUE_TO_TRY;
                    while ( player != playerEnd &&
                            _server->users.get(player,findCh) ) player++;
                        // determine number of public and private players:
                    bool isPrivate = (cpp->botClient & 2) != 0;
                    int privatePlayers = _server->privateUsers.card();
                    int publicPlayers = _server->users.card() - privatePlayers;

                    // in case of pure win version (not GFW), server is always 
                    // in a public slot, on XBox it depends if server is dedicated or not.
#if _GAMES_FOR_WINDOWS
#if _ENABLE_DEDICATED_SERVER  
                    // in case of DS, server is in public slot, in other cases in private slot.
                    if (IsDedicatedServer())
                      publicPlayers += 1;
                    else
#endif
                      privatePlayers += 1;
#else
                    publicPlayers += 1;
#endif

                    if ( player == playerEnd ||
                         _server->session.maxPlayers > 0 &&
                        (privatePlayers + publicPlayers >= _server->session.maxPlayers ||
                         !isPrivate && publicPlayers >= _server->session.maxPlayers - _server->session.maxPrivate) )
                        result = CRSessionFull;
                    else {                  // OK <= new player (at least new FlashPoint run)
                        ch = peer->getPool()->createChannel(distant,peer);
                        if ( !ch ) {
                            result = CRError; // undetermined error
                            RptF("NetServer: createChannel() failed => cannot insert a new player");
                            }
                        else {              // NetChannel is OK
#ifdef NET_LOG_SERVER
#  ifdef NET_LOG_BRIEF
                            NetLog("Ch(%u):acc(%u.%u.%u.%u:%u,'%s',%d,%d)",
                                   ch->getChannelId(),(unsigned)IP4(distant),(unsigned)IP3(distant),(unsigned)IP2(distant),(unsigned)IP1(distant),(unsigned)PORT(distant),
                                   cpp->name,cpp->uniqueID,player);
#  else
                            NetLog("Channel(%u): new player was accepted (from %u.%u.%u.%u:%u), name = '%s', sentID = %d, validID = %d",
                                   ch->getChannelId(),(unsigned)IP4(distant),(unsigned)IP3(distant),(unsigned)IP2(distant),(unsigned)IP1(distant),(unsigned)PORT(distant),
                                   cpp->name,cpp->uniqueID,player);
#  endif
#endif
                            _server->users.put(player,ch);
                            ChannelSupport sup;
                            sup.m_id = player;
                            _server->m_support.put(sockaddrKey(distant),sup);
                            if ( isPrivate ) _server->privateUsers.put(player);
                            //_server->logUsers();
                            DoAssert( _server->users.checkIntegrity() );
                            int index = _server->_createPlayers.Add();
                            CreatePlayerInfo &info = _server->_createPlayers[index];
                            info.player = player;
                            if ( (info.botClient = ((cpp->botClient & 1) != 0)) )
                                _server->botId = player;
                            info.privateSlot = isPrivate;
                            strncpy(info.name,cpp->name,sizeof(info.name));
                            info.name[sizeof(info.name)-1] = (char)0;
                            if ( strcmp(cpp->name,info.name) )
                                RptF("NetServer: name of a new player is too long => truncating to '%S'",info.name);
                            LogF("NetServer: new player (waiting for ProcessPlayers) - session.numPlayers=%d, playerId=%d, bot=%d, name='%s', |users|=%u",
                                 _server->session.numPlayers,info.player,(int)info.botClient,info.name,_server->users.card());
                            strncpy(info.mod,cpp->mod,sizeof(info.mod));
                            info.mod[sizeof(info.mod)-1] = (char)0;
                            ch->setProcessRoutine(serverReceive);
#if VOICE_OVER_NET
                            if ( von.NotNull() && von->getServer().NotNull() )
                              von->getServer()->connect(player);
#endif
                            }
                        }
                    poolLock.leave();
                    }
                _server->leaveUsr();
                    // .. and finally send acknowledgement back to the new user
                AckPlayerPacket app;
                app.magic = MAGIC_ACK_PLAYER;
                app.result = result;
                app.playerNo = player;      // for future reconnections..
                  // ch == NULL in case of failure (but I'm going to send an answer anyway)
                Ref<NetMessage> out = NetMessagePool::pool()->newMessage(sizeof(AckPlayerPacket),ch ? ch : msg->getChannel());
                if ( out ) {
                    if ( !ch ) out->setDistant(distant);
                    out->setFlags(MSG_ALL_FLAGS,MSG_MAGIC_FLAG|(ch?MSG_VIM_FLAG:MSG_FROM_BCAST_FLAG));
                    out->setData((unsigned8*)&app,sizeof(app));
                    out->send(true);        // urgent message
                    }
                if ( ch )                   // initial hand-shake (to initialize band-width..).
                    ch->checkConnectivity(0);
                }
            break;

        }

    return nsNoMoreCallbacks;
}

//---------------------------------------------------------------------------
//  NetSessionEnum:

void NetSessionEnum::sendRequest ( NetChannel *br )
{
  Assert( br );
  struct sockaddr_in addr;
  EnumPacket request;
  request.magic = MAGIC_ENUM_REQUEST;
  request.magicApplication = magicApp;
  request.source = source;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_BROADCAST;
    // server enumeration can sit on any <XBOX_BCAST_PORT,XBOX_BCAST_PORT+NN_NUM_PORTS_TO_TRY) port:
  unsigned port;
  for ( port = XBOX_BCAST_PORT; port < XBOX_BCAST_PORT + NN_NUM_PORTS_TO_TRY; port++ )
  {
    addr.sin_port = htons(port);
    Ref<NetMessage> msg = NetMessagePool::pool()->newMessage(sizeof(EnumPacket),br);
    if ( msg )
    {
      msg->setDistant(addr);
      msg->setFlags(MSG_ALL_FLAGS,MSG_TO_BCAST_FLAG|MSG_MAGIC_FLAG);
      msg->setData((unsigned8*)&request,sizeof(EnumPacket));
      msg->send();
    }
  }
}

//---------------------------------------------------------------------------
//  NetClient:

ConnectResult NetClient::Init ( RString address, RString password, bool botClient, bool privateClient, int &port,
                                RString player, MPVersionInfo &versionInfo, int magic )
{
#ifdef NET_LOG_CLIENT
#  ifdef NET_LOG_BRIEF
  NetLog("Cli(%s,%d)",
         address.GetLength()?(const char*)address:"none",port);
#  else
  NetLog("NetClient::Init: address=%s, port=%d",
         address.GetLength()?(const char*)address:"none",port);
#  endif
#endif
  enterSnd();
  if ( channel )                            // already connected => error
  {
    leaveSnd();
    return CRError;
  }
  magicApp = magic;
  RString ip;
  struct sockaddr_in daddr;
  if ( address.GetLength() == 0 )           // use "localhost"
  {
    char buf[64];
    ErrF("Invalid server IP address");      // this shouldn't happen on Xbox
    sprintf(buf,"127.0.0.1:%d",port);
    address = buf;
  }
    // Xbox: address should contain valid IP address in text format!
  decodeURLAddress(address,ip,port);        // ip = "ip4.ip3.ip2.ip1", port = "port"
  if ( !getHostAddress(daddr,(const char*)ip,port) )
  {
    leaveSnd();
    return CRError;
  }
    // now "daddr" contains valid IP address..
  sessionTerminated = false;
  whySessionTerminated = NTROther;

    // create a new NetChannel for client <-> server communication:
  poolLock.enter();
  if ( getPool() )
    channel = getPool()->createChannel(daddr,GET_CLIENT_PEER());
  if ( !channel )
  {
    poolLock.leave();
    leaveSnd();
    return CRError;
  }
  poolLock.leave();

  channel->setProcessRoutine(clientReceive,channel);

  amIBot = botClient;

    // put together the "CreatePlayer" message:
  CreatePlayerPacket packet;
  packet.magic = MAGIC_CREATE_PLAYER;
  packet.magicApplication = magicApp;
  strncpy(packet.name,player,LEN_PLAYER_NAME);
  packet.name[LEN_PLAYER_NAME-1] = (char)0;
  strncpy(packet.mod,versionInfo.mod,MOD_LENGTH);
  packet.mod[MOD_LENGTH-1] = (char)0;
  strncpy(packet.password,password,LEN_PASSWORD_NAME);
  packet.password[LEN_PASSWORD_NAME-1] = (char)0;
  packet.actualVersion = versionInfo.versionActual;
  packet.requiredVersion = versionInfo.versionRequired;
packet.botClient = (botClient ? 1 : 0) + (privateClient ? 2 : 0);
  packet.uniqueID = (int32)(getSystemTime() & 0x7fffffff) + UNIQUE_TO_TRY + RESERVED_IDS;
#ifdef NET_LOG_CLIENT
#  ifdef NET_LOG_BRIEF
  NetLog("Ch(%u):cli(%u.%u.%u.%u:%u,'%s',%d,%d)",
         channel->getChannelId(),
         (unsigned)IP4(daddr),(unsigned)IP3(daddr),(unsigned)IP2(daddr),(unsigned)IP1(daddr),(unsigned)PORT(daddr),
         (const char*)player,botClient?1:0,packet.uniqueID);
#  else
  NetLog("Channel(%u): NetClient::Init: server=%u.%u.%u.%u:%u, player='%s', bot=%d, magic=%d, playerID=%d",
         channel->getChannelId(),
         (unsigned)IP4(daddr),(unsigned)IP3(daddr),(unsigned)IP2(daddr),(unsigned)IP1(daddr),(unsigned)PORT(daddr),
         (const char*)player,botClient?1:0,magic,packet.uniqueID);
#  endif
#endif
    // .. and send it to the server:
  ackPlayer = CRNone;
  // server will receive this message via it's control channel so I must not use VIM flag!
  // I'll send it multiple times if necessary..

    // now I have to wait to server's response..
  unsigned64 now = getSystemTime();
  unsigned64 next = now;                    // re-send time (init = the 1st try)
  unsigned64 timeout = now + 1000 * ACK_PLAYER_TIMEOUT;
  do
  {
    if ( now >= next )
    {
      Ref<NetMessage> msg = NetMessagePool::pool()->newMessage(sizeof(packet),channel);
      if ( !msg )
      {
        leaveSnd();
        return CRError;
      }
      msg->setFlags(MSG_ALL_FLAGS,MSG_TO_BCAST_FLAG|MSG_MAGIC_FLAG);
      msg->setData((unsigned8*)&packet,sizeof(packet));
      msg->send(true);                      // urgent
      next = now + 1000 * CREATE_PLAYER_RESEND;
    }
    leaveSnd();
    SLEEP_MS(NET_CHECK_WAIT);
    enterSnd();
    ProgressRefresh();
    now = getSystemTime();
  } while ( ackPlayer == CRNone && now < timeout );

  if ( ackPlayer != CRNone )                // initial hand-shake (to initialize band-width)
  {
    channel->checkConnectivity(0);
    struct sockaddr_in distant;
    channel->getDistantAddress(distant);
    port = ntohs(distant.sin_port);
  }

#ifdef NET_LOG_CLIENT
#  ifdef NET_LOG_BRIEF
  NetLog("Ch(%u):cli(%s,%.3f,%d)",
         channel->getChannelId(),
         (ackPlayer == CRNone)?"failed":"connected",1.e-6*(now-timeout+1000*ACK_PLAYER_TIMEOUT),playerNo);
#  else
  NetLog("Channel(%u): NetClient::Init: %s after %.3f seconds (playerID=%d)",
         channel->getChannelId(),
         (ackPlayer == CRNone)?"failed":"connected",1.e-6*(now-timeout+1000*ACK_PLAYER_TIMEOUT),playerNo);
#  endif
#endif
  ConnectResult result = (ackPlayer == CRNone) ? CRError : (ConnectResult)ackPlayer;
  leaveSnd();
  return result;
}

void NetClient::ChangeVoiceMask ( VoiceMask *voiceMask )
{
#if VOICE_OVER_NET
  if ( !von ) return;
  if ( voiceMask )
    von->changeVoiceMask(*voiceMask);
#endif
}

//---------------------------------------------------------------------------
//  NetPeerToPeerServer, -Request:

#ifdef PEER2PEER

NetStatus p2pReceive ( NetMessage *msgPtr, NetStatus event, void *data )
{
  Ref<NetMessage> msg = msgPtr;
  if ( !_p2pServer && !_p2pClient ||
       !msg || msg->getLength() < 4 ||
       !(msg->getFlags() & MSG_MAGIC_FLAG) )
    return nsNoMoreCallbacks;               // fatal message error

  unsigned32 magic = *(unsigned32*)msg->getData();
  struct sockaddr_in distant;
  msg->getDistant(distant);

  switch ( magic )
  {
    case MAGIC_P2P_REQUEST:                 // incoming request
    {
      if ( !_p2pServer || !_p2pServer->m_valid )
        return nsNoMoreCallbacks;
        // create a new channel if no one exists..
      int channel;
      RefD<NetChannel> ch;
      _p2pServer->enterCh();
      ChannelSupport *sup = _p2pServer->m_support.get(sockaddrKey(distant));
      if ( sup )
        _p2pServer->m_channels.get(channel=sup->m_id,ch);
      else
        ch = getPool()->createChannel(distant,GET_P2P_PEER());
      if ( !ch )
      {
        _p2pServer->leaveCh();
        break;
      }
      if ( !sup )                           // create a new channel:
      {
        ch->setProcessRoutine(p2pReceive,ch);
        channel = _p2pServer->m_nextChannel++;
        _p2pServer->m_channels.put(channel,ch);
        ChannelSupport newSup;
        newSup.m_id = channel;
        _p2pServer->m_support.put(sockaddrKey(distant),newSup);
      }
      _p2pServer->leaveCh();
        // everything is OK => pass request to the upper layer
      _p2pServer->enterRcv();
      msg->next = _p2pServer->m_received;
      _p2pServer->m_received = msg;
      _p2pServer->leaveRcv();
      break;
    }
    case MAGIC_P2P_ANSWER:                  // incoming answer
    case MAGIC_P2P_NEGATIVE:                // error message
    {
      if ( !_p2pClient || !_p2pClient->m_valid )
        return nsNoMoreCallbacks;
      unsigned flags = msg->getFlags();
      _p2pClient->enterRcv();
      if ( flags & MSG_PART_FLAG )          // partial message (to be merged)
        if ( flags & MSG_CLOSING_FLAG )     // the last message-part to be merged..
        {
          DoAssert( _p2pClient->m_split.NotNull() );
          _p2pClient->m_lastSplit->next = msg;
          msg->next = NULL;
          msg = mergeMessageList(_p2pClient->m_split);
          _p2pClient->m_split = NULL;
        }
        else                                // another message-part => remember it!
        {
          if ( !_p2pClient->m_split )
            _p2pClient->m_split = msg;
          else
            _p2pClient->m_lastSplit->next = msg;
          (_p2pClient->m_lastSplit = msg)->next = NULL;
          _p2pClient->leaveRcv();
          break;
        }
      msg->next = _p2pClient->m_received;
      _p2pClient->m_received = msg;
      _p2pClient->leaveRcv();
    }
  }

  return nsNoMoreCallbacks;
}


NetPeerToPeerServer::NetPeerToPeerServer ( const XNKID &kid ) : PeerToPeerServer(kid)
{
  m_nextChannel = 0;
  setPeerToPeerServer(this);                // implicitly starts listening at the p2p port..
  if ( _p2pServer )
    m_valid = true;
}

NetPeerToPeerServer::~NetPeerToPeerServer ()
{
  // ??? TODO: cancel all pending jobs ???
  setPeerToPeerServer(NULL);
}

bool NetPeerToPeerServer::SendAnswer ( int channel, BYTE *data, int dataSize )
{
  if ( !m_valid || dataSize < 0 ) return false;
  enterCh();
  if ( !m_channels.presentKey(channel) )
  {
    leaveCh();
    return false;
  }
  RefD<NetChannel> ch;
  m_channels.get(channel,ch);
  unsigned fl = MSG_VIM_FLAG | MSG_MAGIC_FLAG;
  int maxMessage = GET_P2P_PEER()->maxMessageData();
  dataSize += sizeof(unsigned32);           // message length including the "magic" prefix
  Ref<NetMessage> msg;

  if ( dataSize > maxMessage )              // split too big message:
  {
    int toSent = dataSize;
    int packet;
    fl |= MSG_PART_FLAG;
    bool magic = true;
    do
    {
      packet = (toSent > maxMessage) ? maxMessage : toSent;
      toSent -= packet;
      msg = NetMessagePool::pool()->newMessage(packet,ch);
      if ( !msg )
      {
        leaveCh();
        RptF("NetPeerToPeer: pool()->newMessage failed when sending to %d",channel);
        return false;
      }
      msg->setFlags(MSG_ALL_FLAGS,fl | (toSent ? 0 : MSG_CLOSING_FLAG));
      msg->setOrderedPrevious();
      if ( magic )                          // first fragment => sent "magic" first
      {
        magic = false;
        msg->setLength(packet);
        *((unsigned32*)msg->getData()) = MAGIC_P2P_ANSWER;
        memcpy(((unsigned32*)msg->getData())+1,data,packet-sizeof(unsigned32));
        data += packet-sizeof(unsigned32);
      }
      else
      {
        msg->setData(data,packet);
        data += packet;
      }
      msg->send(false);
    } while ( toSent );
  }
  else                                      // small message
  {
    msg = NetMessagePool::pool()->newMessage(dataSize,ch);
    if ( !msg )
    {
      leaveCh();
      RptF("NetPeerToPeer: pool()->newMessage failed when sending to %d",channel);
      return false;
    }
    msg->setFlags(MSG_ALL_FLAGS,fl);
    msg->setOrderedPrevious();              // VIM is set ORDERED automatically
    msg->setLength(dataSize);
    *((unsigned32*)msg->getData()) = MAGIC_P2P_ANSWER;
    if ( dataSize > sizeof(unsigned32) )
      memcpy(((unsigned32*)msg->getData())+1,data,dataSize-sizeof(unsigned32));
    msg->send(false);
  }

  leaveCh();
  return true;
}

void NetPeerToPeerServer::ProcessIncomingMessages ( PeerToPeerIncomingCallback *callback, void *context )
{
  if ( !m_valid ) return;
  enterRcv();
  Ref<NetMessage> rcv = m_received;
  m_received = NULL;
  leaveRcv();
  if ( !callback ) return;                  // no processing routine was defined => simply throw away the messages
  while ( rcv )
  {
    P2PMsgType type = P2PNegative;
    unsigned32 magic = *(unsigned*)rcv->getData();
    switch ( magic )
    {
      case MAGIC_P2P_REQUEST:
        type = P2PRequest;
        break;
      case MAGIC_P2P_ANSWER:
        type = P2PAnswer;
        break;
    }
    struct sockaddr_in distant;
    rcv->getDistant(distant);
    enterCh();
    ChannelSupport *sup = m_support.get(sockaddrKey(distant));
    if ( sup )
      (*callback)(sup->m_id,type,(BYTE*)(((unsigned32*)rcv->getData())+1),rcv->getLength()-sizeof(unsigned32),context);
    leaveCh();
    rcv = rcv->next;
  }
  // !!! TODO: cleanup !!! destroy obsolete channels, etc.
}

//---------------------------------------------------------------------------

NetPeerToPeerClient::NetPeerToPeerClient ( const XNKID &kid, const XNKEY &key, const XNADDR &addr )
  : PeerToPeerClient(kid,key,addr)
{
  m_registered = false;
  if ( _p2pClient ) return;                 // error <= another request is still registered
  int result = XNetRegisterKey(&kid,&key);
  if ( result != 0 )
  {
    RptF("NetPeerToPeerClient: XNetRegisterKey failed with error 0x%x",result);
    return;
  }
  m_registered = true;
  setPeerToPeerClient(this);                // implicitly starts listening at the p2p port..
  if ( _p2pClient )
    m_valid = true;
}

NetPeerToPeerClient::~NetPeerToPeerClient ()
{
  // ??? TODO: cancel all pending jobs ???
  if ( m_registered )
  {
    XNetUnregisterKey(&m_kid);
    m_registered = false;
  }
  setPeerToPeerClient(NULL);
}

bool NetPeerToPeerClient::SendRequest ( BYTE *data, int dataSize )
{
  if ( !m_valid || !data || dataSize < 0 ) return false;
                                            // fatal error
  IN_ADDR inaddr;
  INT err = XNetXnAddrToInAddr(&m_addr,&m_kid,&inaddr);
  if ( err != 0 )
  {
    RptF("NetPeerToPeerClient: XNetXnAddrToInAddr failed with error 0x%x",err);
    return false;
  }

    // I got valid inet-address: will create NetChannel to server and send the request
  struct sockaddr_in daddr;
  Zero(daddr);
  daddr.sin_addr.S_un = inaddr.S_un;
  daddr.sin_family    = AF_INET;
  daddr.sin_port      = htons(XBOX_P2P_PORT);
  m_channel = getPool()->createChannel(daddr,GET_P2P_PEER());
  if ( !m_channel )
    return false;
  m_channel->setProcessRoutine(p2pReceive,m_channel);

    // now I'm ready to send the request:
  Ref<NetMessage> msg = NetMessagePool::pool()->newMessage(sizeof(unsigned32)+dataSize,m_channel);
  if ( !msg )
    return false;
  msg->setFlags(MSG_ALL_FLAGS,MSG_TO_BCAST_FLAG|MSG_MAGIC_FLAG);
  msg->setLength(sizeof(unsigned32)+dataSize);
  *((unsigned32*)msg->getData()) = MAGIC_P2P_REQUEST;
  memcpy(((unsigned32*)msg->getData())+1,data,dataSize);
  msg->send(true);                          // urgent
  return true;
}

void NetPeerToPeerClient::ProcessIncomingMessages ( PeerToPeerIncomingCallback *callback, void *context )
{
  if ( !m_valid ) return;
  enterRcv();
  Ref<NetMessage> rcv = m_received;
  m_received = NULL;
  leaveRcv();
  if ( !callback ) return;                  // no processing routine was defined => simply throw away the messages
  while ( rcv )
  {
    P2PMsgType type = P2PNegative;
    unsigned32 magic = *(unsigned*)rcv->getData();
    if ( magic == MAGIC_P2P_ANSWER )
      type = P2PAnswer;
    struct sockaddr_in distant;
    rcv->getDistant(distant);
    (*callback)(0,type,(BYTE*)(((unsigned32*)rcv->getData())+1),rcv->getLength()-sizeof(unsigned32),context);
    rcv = rcv->next;
  }
  // ??? TODO: cleanup ???
}

PeerToPeerServer *CreatePeerToPeerServer ( const XNKID &kid )
{
  NetPeerToPeerServer *srv = new NetPeerToPeerServer(kid);
  if ( srv->IsValid() )
    return srv;
  delete srv;
  return NULL;
}

PeerToPeerClient *CreatePeerToPeerClient ( const XNKID &kid, const XNKEY &key, const XNADDR &addr )
{
  NetPeerToPeerClient *cli = new NetPeerToPeerClient(kid,key,addr);
  if ( cli->IsValid() )
    return cli;
  delete cli;
  return NULL;
}

#endif    // PEER2PEER

#if VOICE_OVER_NET

//---------------------------------------------------------------------------
//  Voice recording/replaying:

VoNResult VoiceRecordingStart ()
{
  if ( !von ) return VonError;
  return von->startRecorder();
}

int VoiceRecordingStop ( AutoArray<char,MemAllocSafe> &data )
{
  if ( !von ) return 0;
  return von->stopRecorder(data);
}

VoNResult VoiceReplayStart ( AutoArray<char,MemAllocSafe> &data, int duration )
{
  if ( !von ) return VonError;
  return von->startReplay(data,duration);
}

VoNResult VoiceReplayStatus ()
{
  if ( !von ) return VonError;
  return von->getReplayStatus();
}

VoNResult VoiceReplayStop ()
{
  if ( !von ) return VonError;
  return von->stopReplay();
}

void VoiceDataSent(int pid, int len)
{
  _server->NotifyChannelDataSent(pid,len);
}

NetPeerToPeerChannel *VoiceCreatePeerToPeer()
{
  XboxPeerToPeerChannel *ch = new XboxPeerToPeerChannel();
  if (!ch->Init(XBOX_P2P_PORT))
  {
    // channel init failed
    RptF("Peer to peer channel init failed");
    delete ch;
    return NULL;
  }
  return ch;
}

#else   // VOICE_OVER_NET

//  W/O the VoN system voice recording is not functioning:

VoNResult VoiceRecordingStart ()
{
  return VonError;
}

int VoiceRecordingStop ( AutoArray<char,MemAllocSafe> &data )
{
  return 0;
}

VoNResult VoiceReplayStart ( AutoArray<char,MemAllocSafe> &data, int duration )
{
  return VonError;
}

VoNResult VoiceReplayStatus ()
{
  return VonError;
}

VoNResult VoiceReplayStop ()
{
  return VonError;
}

void VoiceDataSent(int pid, int len)
{
}

#endif  // VOICE_OVER_NET

//---------------------------------------------------------------------------
//  Common implementation (shared with netTransportNet.cpp):

#include "netTransportCommon.hpp"

#endif  // _XBOX_SECURE

#endif  // _ENABLE_MP
